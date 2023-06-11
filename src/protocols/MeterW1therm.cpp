/**
 * 1wire-support for therm sensors supported by linux modules w1-therm
 * functionality similar to 1wirevz
 *
 * starting with kernel 3.18 this needs a
 * dtoverlay=w1-gpio
 * (or with parameter dtoverlay=w1-gpio,gpiopin=4,pullup=1)
 * in /boot/config.txt
 *
 * @copyright Copyright (c) 2015 - 2023, the volkszaehler.org project
 * @license http://www.gnu.org/licenses/glp2.txt GNU Public License v2
 * @author Matthias Behr <mbehr (a) mcbehr.de>
 * */

#include "threads.h"

#include "protocols/MeterW1therm.hpp"
#include <glob.h>

bool MeterW1therm::W1sysHWif::scanW1devices() {
	// scan directory /sys/bus/w1/devices for all devices starting with the W1_THERM
	// supported family codes:
	// W1_THERM_DS18S20 0x10
	// W1_THERM_DS1822 0x22
	// W1_THERM_DS18B20 0x28
	// W1_THERM_DS1825 0x3B
	// W1_THERM_DS28EA00 0x42
	//
	// I'd prefer to scan all devices and check driver for W1_THERM but I don't know how...

	_devices.clear();

	// let's try using glob()
	glob_t glob_res;
	size_t strl = strlen("/sys/bus/w1/devices/");
	if (0 == glob("/sys/bus/w1/devices/"
				  // strange formatting is required because of
				  // https://en.wikipedia.org/wiki/Digraphs_and_trigraphs#C
				  "?"
				  "?"
				  "-*",
				  GLOB_NOSORT, NULL, &glob_res)) {

		// 1wire ID prefixes of supported sensors
		const char *ids[] = {"10", "22", "28", "3b", "42", NULL};

		for (unsigned int i = 0; i < glob_res.gl_pathc; ++i) {
			for (unsigned int j = 0; ids[j] != NULL; j++) {
				if (strlen(glob_res.gl_pathv[i]) > strl &&
					!strncmp(glob_res.gl_pathv[i] + strl, ids[j], 2)) {
					char *str = glob_res.gl_pathv[i] + strl;
					_devices.push_back(std::string(str));
				}
			}
		}

		globfree(&glob_res);
	}

	// for now we return false is the list is empty
	if (_devices.size() > 0)
		return true;
	return false;
}

bool MeterW1therm::W1sysHWif::readTemp(const std::string &device, double &value) {
	bool toret = false;
	// read from /sys/bus/w1/devices/<device>/w1_slave
	std::string dev("/sys/bus/w1/devices/");
	dev.append(device);
	dev.append("/w1_slave");

	FILE *fp;
	if ((fp = fopen(dev.c_str(), "r")) == NULL) {
		print(log_debug, "couldn't open %s for reading", "w1t", dev.c_str());
		return false;
	}

	// check for CRC ok
	char buffer[100];
	if (fgets(buffer, 100, fp)) {
		if (!strstr(buffer, "YES")) { // e.g. 07 01 55 00 7f ff 0c 10 18 : crc=18 YES
			print(log_debug, "CRC not ok from %s (%s)", "w1t", dev.c_str(), buffer);
		} else {
			// crc ok
			// now parse t=<value>
			if (fgets(buffer, 100, fp)) { // e.g. 07 01 55 00 7f ff 0c 10 18 t=16437
				char *pos = strstr(buffer, "t=");
				if (pos) {
					pos += 2;
					value = atof(pos) / 1000;
					toret = true;
					print(log_finest, "read %f from %s (%s)", "w1t", value, dev.c_str(), pos - 2);
				}
			} else {
				print(log_debug, "couldn't read 2nd line from %s", "w1t", dev.c_str());
			}
		}
	} else {
		print(log_debug, "couldn't read 1st line from %s", "w1t", dev.c_str());
	}

	fclose(fp);

	return toret;
}

MeterW1therm::MeterW1therm(const std::list<Option> &options, W1HWif *hwif)
	: Protocol("w1t"), _hwif(hwif) {
	if (!_hwif)
		_hwif = new W1sysHWif();
}

MeterW1therm::~MeterW1therm() {
	if (_hwif)
		delete _hwif;
}

int MeterW1therm::open() {
	if (!_hwif)
		return ERR;
	if (!_hwif->scanW1devices()) {
		print(log_alert, "scanW1devices failed!", name().c_str());
		return ERR;
	}
	print(log_info, "open found %d w1 devices", name().c_str(), _hwif->W1devices().size());
	return SUCCESS;
}

int MeterW1therm::close() { return SUCCESS; }

ssize_t MeterW1therm::read(std::vector<Reading> &rds, size_t n) {
	ssize_t ret = 0;
	if (!_hwif)
		return 0;

	// todo add a scanW1devices() e.g. each minute?

	const std::list<std::string> &list = _hwif->W1devices();

	for (std::list<std::string>::const_iterator it = list.cbegin();
		 it != list.cend() && static_cast<size_t>(ret) < n; ++it) {
		_safe_to_cancel();
		double value;
		if (_hwif->readTemp(*it, value)) {
			print(log_finest, "reading w1 device %s returned %f", name().c_str(), (*it).c_str(),
				  value);
			rds[ret].identifier(new StringIdentifier(*it));
			rds[ret].time();
			rds[ret].value(value);
			++ret;
		} else {
			print(log_debug, "reading w1 device %s failed", name().c_str(), (*it).c_str());
		}
	}

	return ret;
}
