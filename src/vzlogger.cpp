/**
 * Main source file
 *
 * @package vzlogger
 * @copyright Copyright (c) 2011 - 2023, The volkszaehler.org project
 * @license http://www.gnu.org/licenses/gpl.txt GNU Public License
 * @author Steffen Vogel <info@steffenvogel.de>
 */
/*
 * This file is part of volkzaehler.org
 *
 * volkzaehler.org is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * any later version.
 *
 * volkzaehler.org is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with volkszaehler.org. If not, see <http://www.gnu.org/licenses/>.
 */

#include <curl/curl.h>
#include <errno.h>
#include <fcntl.h>
#include <getopt.h>
#include <signal.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sysexits.h>
#include <time.h>
#include <unistd.h>

#include <list>
#include <mutex>
#include <sstream>

#include "Channel.hpp"
#include "CurlSessionProvider.hpp"
#include "Obis.hpp"
#include "PushData.hpp"
#include "threads.h"
#include "vzlogger.h"
#include <Config_Options.hpp>
#include <Meter.hpp>

#ifdef LOCAL_SUPPORT
#include "local.h"
#endif /* LOCAL_SUPPORT */

#ifdef ENABLE_MQTT
#include "mqtt.hpp"
#endif

#include "gitSha1.h"

MapContainer mappings;     // mapping between meters and channels
Config_Options options;    // global application options
size_t gSkippedFailed = 0; // disabled or failed meters

std::stringbuf *gStartLogBuf = 0; // temporay buffer for print until logfile is opened
std::mutex
	m_log; // mutex for central log function, to prevent competed write access from the threads.

/**
 * Command line options
 */
const struct option long_options[] = {
	{"config", required_argument, 0, 'c'},
	{"log", required_argument, 0, 'o'},
	{"foreground", no_argument, 0, 'f'},
#ifdef LOCAL_SUPPORT
	{"httpd", no_argument, 0, 'l'},
	{"httpd-port", required_argument, 0, 'p'},
#endif /* LOCAL_SUPPORT */
	{"register", no_argument, 0, 'r'},
	{"verbose", required_argument, 0, 'v'},
	{"help", no_argument, 0, 'h'},
	{"version", no_argument, 0, 'V'},
	{0, 0, 0, 0} /* stop condition for iterator */
};

/**
 * Descriptions vor command line options
 */
const char *long_options_descs[] = {
	"configuration file",
	"log file",
	"run in foreground, do not daemonize",
#ifdef LOCAL_SUPPORT
	"activate local interface (tiny HTTPd which serves live readings)",
	"TCP port for HTTPd",
#endif /* LOCAL_SUPPORT */
	"register device",
	"enable verbose output",
	"show this help",
	"show version of vzlogger",
	NULL /* stop condition for iterator */
};

void openLogfile(bool skip_lock = false) {
	FILE *logfd = fopen(options.log().c_str(), "a");

	if (logfd == NULL) {
		print(log_alert, "opening logfile \"%s\" failed: %s", (char *)0, options.log().c_str(),
			  strerror(errno));
		exit(EX_CANTCREAT);
	}

	if (!skip_lock)
		m_log.lock();

	if (gStartLogBuf) {
		// log current console output to logfile as we missed the start
		fprintf(logfd, "%s", gStartLogBuf->str().c_str());
		auto temp = gStartLogBuf;
		gStartLogBuf = 0;
		delete temp;
	}

	options.logfd(logfd);
	if (!skip_lock)
		m_log.unlock();
	print(log_debug, "Opened logfile %s", (char *)0, options.log().c_str());
}

void closeLogfile(bool skip_lock = false) {
	if (!skip_lock)
		m_log.lock();
	if (options.logfd()) {
		fclose(options.logfd());
		options.logfd(NULL);
	}
	if (!skip_lock)
		m_log.unlock();
}

/**
 * Print error/debug/info messages to stdout and/or logfile
 *
 * @param id could be NULL for general messages
 * @todo integrate into syslog
 */
void print(log_level_t level, const char *format, const char *id, ...) {
	if (level > options.verbosity()) {
		return; /* skip message if its under the verbosity level */
	}

	struct timeval now;
	struct tm *timeinfo;
	char prefix[24];
	size_t pos = 0;

	gettimeofday(&now, NULL);
	timeinfo = localtime(&now.tv_sec);

	/* format timestamp */
	pos += strftime(prefix + pos, 18, "[%b %d %H:%M:%S]", timeinfo);

	/* format section */
	if (id) {
		snprintf(prefix + pos, 8, "[%s]", (char *)id);
	}

	va_list args;
	va_start(args, id);
	/* print to stdout/stderr */
	if (getppid() != 1) { /* running as fork in background? */
		FILE *stream = (level > 0) ? stdout : stderr;

		m_log.lock(); // safe write access for competed access from other thread
		fprintf(stream, "%-24s", prefix);
		vfprintf(stream, format, args);
		fprintf(stream, "\n");
		m_log.unlock(); // release mutex
	}
	va_end(args);

	va_start(args, id);
	/* append to logfile */
	m_log.lock(); // safe write access for competed access from other thread
	if (options.logfd()) {
		fprintf(options.logfd(), "%-24s", prefix);
		vfprintf(options.logfd(), format, args);
		fprintf(options.logfd(), "\n");
		fflush(options.logfd());
	} else if (gStartLogBuf) {
		char buf[500];
		int bufUsed;
		bufUsed = snprintf(buf, 500, "%-24s", prefix);
		bufUsed += vsnprintf(buf + bufUsed, bufUsed < 500 ? 500 - bufUsed : 0, format, args);
		bufUsed += snprintf(buf + bufUsed, bufUsed < 500 ? 500 - bufUsed : 0, "\n");
		gStartLogBuf->sputn(buf, bufUsed < 500 ? bufUsed : 500);
	}
	m_log.unlock();
	va_end(args);
}

/**
 * Print available options, protocols and OBIS aliases
 */
void show_usage(char *argv[]) {
	const char **desc = long_options_descs;
	const struct option *op = long_options;

	printf("Usage: %s [options]\n\n", argv[0]);

	/* command line options */
	printf("  following options are available:\n");
	while (op->name && desc) {
		printf("\t-%c, --%-12s\t%s\n", op->val, op->name, *desc);
		op++;
		desc++;
	}

	/* protocols */
	printf("\n  following protocol types are supported:\n");
	for (const meter_details_t *it = meter_get_protocols(); it->id > 0; it++) {
		printf("\t%-12s\t%s\n", it->name, it->desc);
	}

	/* obis aliases */
	printf("\n  following OBIS aliases are available:\n");
	char obis_str[OBIS_STR_LEN];
	for (obis_alias_t *it = obis_get_aliases(); it->name != NULL; it++) {
		it->id.unparse(obis_str, OBIS_STR_LEN);
		printf("\t%-17s%-31s%-22s\n", it->name, it->desc, obis_str);
	}

	printf("\n  following APIs are supported:\n");
	printf("\tvolkszaehler-api\n");
	printf("\tmysmartgrid-api\n");

	/* footer */
	printf("\n%s - volkszaehler.org logging utility\n", PACKAGE_STRING);
	printf("by Steffen Vogel <stv0g@0l.de>\n");
	printf("send bugreports to %s\n", PACKAGE_BUGREPORT);
}

/**
 * Fork process to background
 *
 * @link http://www.enderunix.org/docs/eng/daemon.php
 */
void daemonize() {
	if (getppid() == 1) {
		return; /* already a daemon */
	}

	int i = fork();
	if (i < 0) {
		exit(EX_OSERR); /* fork error */
	} else if (i > 0) {
		exit(EXIT_SUCCESS); /* parent exits */
	}

	/* child (daemon) continues */
	setsid(); /* obtain a new process group */

	for (i = getdtablesize(); i >= 0; --i) {
		close(i); /* close all descriptors */
	}

	/* handle standart I/O */
	i = open("/dev/null", O_RDWR);
	if (dup(i) < 0) {
	}
	if (dup(i) < 0) {
	}

	if (chdir("/") < 0) {
	} /* change working directory */
	umask(0022);

	/* ignore signals from parent tty */
	struct sigaction action;
	sigemptyset(&action.sa_mask);
	action.sa_flags = 0;
	action.sa_handler = SIG_IGN;

	sigaction(SIGCHLD, &action, NULL); /* ignore child */
	sigaction(SIGTSTP, &action, NULL); /* ignore tty signals */
	sigaction(SIGTTOU, &action, NULL);
	sigaction(SIGTTIN, &action, NULL);
}

/**
 * signal handler that is called e.g on
 * an intended stop by systemd.
 * We just indicate to the threads that they should
 * end.
 * Threads itself gets joined in main()
 */

volatile bool mainLoopEndThreads = false;

void signalHandlerQuit(int sig) {
	// this is a signal handler. We're only allowed to call
	// async-signal-safe functions. see e.g. man 7 signal-safety
	mainLoopEndThreads = true;
	// mappings.quit(sig);
	end_push_data_thread();
#ifdef ENABLE_MQTT
	end_mqtt_client_thread();
#endif
}

/**
 * signal handler for re-opening logfile.
 * the actual operation is carried out in main(),
 * because the required operations are not signal-safe.
 */

volatile bool mainLoopReopenLogfile = false;

void signalHandlerReOpenLog(int) { mainLoopReopenLogfile = true; }

/**
 * Parse options from command line
 *
 * @param options pointer to structure for options
 * @return int 0 on succes, <0 on error
 */
int config_parse_cli(int argc, char *argv[], Config_Options *options) {
	while (1) {
		int c = getopt_long(argc, argv, "c:o:p:lhrVfv:", long_options, NULL);

		/* detect the end of the options. */
		if (c == -1)
			break;

		switch (c) {
		case 'v':
			options->verbosity(atoi(optarg));
			break;

#ifdef LOCAL_SUPPORT
		case 'l':
			options->local(1);
			break;

		case 'p': /* port for local interface */
			options->port(atoi(optarg));
			break;
#endif /* LOCAL_SUPPORT */

		case 'f':
			options->foreground(1);
			break;

		case 'c': /* config file */
			options->config(optarg);
			break;

		case 'o': /* log file */
			options->log(optarg);
			break;

		case 'V':
			printf("%s\n", VERSION);
			printf(" based on git version: %s\n", g_GIT_SHALONG);
			printf(" last commit date: %s\n", g_GIT_LAST_COMMIT_DATE);
			exit(EXIT_SUCCESS);
			break;

		case 'r':
			options->doRegistration(1);
			break;

		case '?':
		case 'h':
		default:
			show_usage(argv);
			if (c == '?') {
				exit(EXIT_FAILURE);
			} else {
				exit(EXIT_SUCCESS);
			}
		}
	}

	return SUCCESS;
}

/*---------------------------------------------------------------------*/
/**
 * @brief send api-device registration message
 **/
/*---------------------------------------------------------------------*/
void register_device() {
	// using global variable:  mappings	 mapping between meters and channels */
	// using global variable:  options	 global application options */
	try {
		/* open connection meters & start threads */
		for (MapContainer::iterator it = mappings.begin(); it != mappings.end(); it++) {
			it->registration();
		}
	} catch (std::exception &e) {
		print(log_alert, "Registration failed for %s", "", e.what());
	}
}
#ifndef NOMAIN

/**
 * The application entrypoint
 */
int main(int argc, char *argv[]) {
	pthread_t _pushdata_thread = 0;
#ifdef ENABLE_MQTT
	pthread_t _mqtt_client_thread = 0;
#endif

	// bind signal handler for exiting vzlogger
	struct sigaction quitaction;
	sigemptyset(&quitaction.sa_mask);
	quitaction.sa_flags = 0;
	quitaction.sa_handler = signalHandlerQuit;
	sigaction(SIGINT, &quitaction, NULL);  /* catch ctrl-c from terminal */
	sigaction(SIGHUP, &quitaction, NULL);  /* catch hangup signal */
	sigaction(SIGTERM, &quitaction, NULL); /* catch kill signal */

	// signal for re-opening logfile
	struct sigaction reopenaction;
	sigemptyset(&reopenaction.sa_mask);
	reopenaction.sa_flags = 0;
	reopenaction.sa_handler = signalHandlerReOpenLog;
	// sigaction() follows in conditional below.

	gStartLogBuf = new std::stringbuf;

#ifdef LOCAL_SUPPORT
	/* webserver for local interface */
	struct MHD_Daemon *httpd_handle = NULL;
#endif /* LOCAL_SUPPORT */

	/* initialize ADTs and APIs */
	//	curl_global_init(CURL_GLOBAL_ALL);

	/* parse command line and file options */
	// TODO command line should have a higher priority as file
	if (config_parse_cli(argc, argv, &options) != SUCCESS) {
		return EX_USAGE;
	}

	// always (that's why log_alert is used) print version info to log file:
	print(log_alert, "vzlogger v%s based on %s from %s started.", "main", VERSION, g_GIT_SHALONG,
		  g_GIT_LAST_COMMIT_DATE);

	// mappings = (MapContainer::Ptr)(new MapContainer());
	try {
		options.config_parse(mappings);
	} catch (std::exception &e) {
		std::stringstream oss;
		oss << e.what();
		print(log_alert, "Failed to parse configuration due to: %s", NULL, oss.str().c_str());
		return EX_CONFIG;
	}

	// make sure command line options override config settings, just re-parse
	optind = 0; // otherwise the getopt_long doesn't parse from start!
	config_parse_cli(argc, argv, &options);

	print(log_alert, "log level is %d", "main", options.verbosity());

	curlSessionProvider = new CurlSessionProvider();

	// Register vzlogger
	if (options.doRegistration()) {
		register_device();
		return (0);
	}

	print(log_debug, "local=%d", "main", options.local());

	if (!options.foreground()) {
		print(log_info, "Daemonize process...", (char *)0);
		daemonize();
	} else {
		print(log_info, "Process not daemonized...", (char *)0);
	}

	/* open logfile */
	if (options.log() != "") {
		openLogfile();
		sigaction(SIGUSR1, &reopenaction, NULL); // catch SIGUSR1
	} else {
		// stop temp logging, continue logging to console only
		auto temp = gStartLogBuf;
		gStartLogBuf = 0;
		delete temp;
	}

	if (mappings.size() <= 0) {
		print(log_alert, "No meters found - quitting!", (char *)0);
		return EXIT_FAILURE;
	}

	if (options.pushDataServer()) {
		pushDataList = new PushDataList();
		int ret = pthread_create(&_pushdata_thread, NULL, push_data_thread,
								 (void *)options.pushDataServer()); // todo error handling?
		if (ret)
			print(log_error, "Error %d creating pushdata_thread!", "push", ret);
		else
			print(log_finest, "pushdata_thread created.", "push");
	} else {
		print(log_finest, "No pushDataServer defined.", "push");
	}

#ifdef ENABLE_MQTT
	if (mqttClient) {
		int ret = pthread_create(&_mqtt_client_thread, NULL, mqtt_client_thread, (void *)0);
		if (ret)
			print(log_error, "Error %d creating mqtt_client_thread!", "mqtt", ret);
		else
			print(log_finest, "mqtt_client_thread created.", "mqtt");
	}
#endif

	print(log_debug, "===> Start meters", "");
	try {
		// open connection meters & start threads
		for (MapContainer::iterator it = mappings.begin(); it != mappings.end(); it++) {
			it->start();
			if (!it->running()) {
				gSkippedFailed++;
			}
		}

		// quit if not at least one meter is enabled and working
		if (mappings.size() - gSkippedFailed <= 0) {
			print(log_alert, "No functional meters found - quitting!", (char *)0);
			return EXIT_FAILURE;
		}

#ifdef LOCAL_SUPPORT
		// start webserver for local interface
		if (options.local()) {
			print(log_info, "Starting local interface HTTPd on port %i", "http", options.port());
			httpd_handle =
				MHD_start_daemon(MHD_USE_THREAD_PER_CONNECTION, options.port(), NULL, NULL,
								 &handle_request, (void *)&mappings, MHD_OPTION_END);
		}
#endif /* LOCAL_SUPPORT */
	} catch (std::exception &e) {
		print(log_alert, "Startup failed: %s", "", e.what());
		exit(1);
	}
	print(log_debug, "Startup done.", "");

	try {
		bool cancelledThreads = false;
		bool oneRunning;
		do {
			oneRunning = false;
			// see whether at least one thread is still running:
			for (MapContainer::iterator it = mappings.begin(); it != mappings.end(); it++) {
				if (it->running()) {
					oneRunning = true;
				}
			}
			// shall we stop the threads?
			if (mainLoopEndThreads and !cancelledThreads) {
				print(log_info, "main loop indicating all mappings to quit", "");
				cancelledThreads = true;
				for (MapContainer::iterator it = mappings.begin(); it != mappings.end(); it++) {
					if (it->running()) {
						it->cancel();
					}
				}
			} else { // !mainLoopEndThreads or cancelledThread already
				if (oneRunning) {
					// at least one thread still running and we're not asked
					// to stop yet. So let's wait a bit to avoid busy looping
					if (mainLoopEndThreads) {
						print(log_info, "main loop waiting for running threads...", "");
					}
					::sleep(1); // 1s should be ok. will introduce a shutdown latency >1s
				}
			}
			if (mainLoopReopenLogfile) {
				mainLoopReopenLogfile = false;
				print(log_info, "closing logfile for re-opening (requested with SIGUSR1)", "");
				m_log.lock();
				closeLogfile(true);
				openLogfile(true);
				m_log.unlock();
				print(log_info, "re-opened logfile (requested with SIGUSR1)", "");
			}
		} while (oneRunning);
	} catch (std::exception &e) {
		print(log_error, "Main loop failed for %s", "", e.what());
	}
	print(log_debug, "Server stopped.", "");

#ifdef LOCAL_SUPPORT
	/* stop webserver */
	if (httpd_handle) {
		print(log_finest, "Waiting for httpd to stop...", "");
		MHD_stop_daemon(httpd_handle);
		print(log_finest, "httpd stopped", "");
	}
#endif /* LOCAL_SUPPORT */

	/* householding */
	// curl_global_cleanup();

	if (pushDataList) {

		end_push_data_thread();
		print(log_finest, "Waiting for pushdata_thread to stop...", "");
		pthread_join(_pushdata_thread, NULL);
		print(log_finest, "pushdata_thread stopped", "");

		if (pushDataList) {
			delete pushDataList;
			pushDataList = 0;
			print(log_finest, "deleted pushdataList", "");
		}
	}

#ifdef ENABLE_MQTT
	if (_mqtt_client_thread) {
		end_mqtt_client_thread();
		print(log_finest, "Waiting for mqtt_client_thread to stop...", "mqtt");
		pthread_join(_mqtt_client_thread, NULL);
		print(log_finest, "mqtt_client_thread stopped", "mqtt");
	}
	if (mqttClient) {
		delete mqttClient;
		mqttClient = 0;
		print(log_finest, "deleted mqttClient", "mqtt");
	}
#endif

	if (curlSessionProvider) {
		print(log_finest, "Trying to delete curlSessionProvider...", "");
		delete curlSessionProvider;
		curlSessionProvider = 0;
		print(log_finest, "deleted curlSessionProvider", "");
	}

	closeLogfile();

	return EXIT_SUCCESS;
}
#endif
