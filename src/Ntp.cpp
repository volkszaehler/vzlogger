
#include <string.h>
#include <time.h>

#include "pico/stdlib.h"
#include "pico/cyw43_arch.h"

#include "lwip/dns.h"
#include "lwip/pbuf.h"
#include "lwip/udp.h"

#define NTP_SERVER "pool.ntp.org"
#define NTP_MSG_LEN 48
#define NTP_PORT 123
#define NTP_DELTA 2208988800 // seconds between 1 Jan 1900 and 1 Jan 1970
#define NTP_TEST_TIME (30 * 1000)
#define NTP_RESEND_TIME (10 * 1000)

#include <common.h>
#include <Ntp.hpp>

static void ntp_recv(void *arg, struct udp_pcb *pcb, struct pbuf *p, const ip_addr_t *addr, u16_t port);
static int64_t ntp_failed_handler(alarm_id_t id, void *user_data);
static void ntp_dns_found(const char *hostname, const ip_addr_t *ipaddr, void *arg);

Ntp::Ntp() : dns_request_sent(false), theTime(0)
{
  ntp_pcb = udp_new_ip_type(IPADDR_TYPE_ANY);
  if (!ntp_pcb)
  {
    // TODO exception
    print(log_error, "NTP - Failed to create pcb", "");
  }
  udp_recv(ntp_pcb, ntp_recv, this);
}

Ntp::~Ntp()
{
  // TODO cleanup
}

void Ntp::setServerAddress(const ip_addr_t * ipaddr) { ntp_server_address = *ipaddr; }
const ip_addr_t * Ntp::getServerAddress() { return &ntp_server_address; }

time_t Ntp::queryTime()
{
  // TODO TGE ??? if (absolute_time_diff_us(get_absolute_time(), ntp_test_time) < 0 && ! dns_request_sent)
  if (! dns_request_sent)
  {
    print(log_debug, "Setting timer ...", "");
    // Set alarm in case udp requests are lost
    ntp_resend_alarm = add_alarm_in_ms(NTP_RESEND_TIME, ntp_failed_handler, this, true);

    print(log_debug, "Resolving NTP server %s ...", "", NTP_SERVER);
    cyw43_arch_lwip_begin();
    int err = dns_gethostbyname(NTP_SERVER, &ntp_server_address, ntp_dns_found, this);
    cyw43_arch_lwip_end();

    print(log_debug, "DNS resolution of NTP server triggered ...", "");
    dns_request_sent = true;
    if (err == ERR_OK)
    {
      this->request(); // Cached result
    }
    else if (err != ERR_INPROGRESS)
    {
      // ERR_INPROGRESS means expect a callback
      // TODO exception
      print(log_error, "DNS request failed", "");
      this->result(-1, NULL);
    }
  }

  // Wait for completion
  while(dns_request_sent)
  {
    print(log_debug, "Waiting for NTP time ...", "");
    sleep_ms(1000);
  }
  return theTime;
}

// Called with results of operation
void Ntp::result(int status, time_t *result)
{
  print(log_debug, "NTP result arrived", "");
  if (status == 0 && result)
  {
    struct tm *utc = gmtime(result);
    print(log_debug, "got ntp response: %02d/%02d/%04d %02d:%02d:%02d", "", utc->tm_mday, utc->tm_mon + 1, utc->tm_year + 1900,
          utc->tm_hour, utc->tm_min, utc->tm_sec);
    theTime = *result;
  }

  if (ntp_resend_alarm > 0)
  {
    print(log_debug, "Cancelling alarm", "");
    cancel_alarm(ntp_resend_alarm);
    ntp_resend_alarm = 0;
  }
  ntp_test_time = make_timeout_time_ms(NTP_TEST_TIME);
  dns_request_sent = false;
}


// Make an NTP request
void Ntp::request()
{
  print(log_debug, "Sending UDP request to NTP Server at %s ...", "", ipaddr_ntoa(&ntp_server_address));
  cyw43_arch_lwip_begin();
  struct pbuf *p = pbuf_alloc(PBUF_TRANSPORT, NTP_MSG_LEN, PBUF_RAM);
  uint8_t *req = (uint8_t *) p->payload;
  memset(req, 0, NTP_MSG_LEN);
  req[0] = 0x1b;
  udp_sendto(ntp_pcb, p, &ntp_server_address, NTP_PORT);
  pbuf_free(p);
  cyw43_arch_lwip_end();
}

static int64_t ntp_failed_handler(alarm_id_t id, void *user_data)
{
  Ntp * ntp = (Ntp *) user_data;
  // TODO exception
  print(log_error, "NTP request failed", "");
  ntp->result(-1, NULL);
  return 0;
}

// Call back with a DNS result
static void ntp_dns_found(const char *hostname, const ip_addr_t *ipaddr, void *arg)
{
  print(log_debug, "Resolved NTP server %s", "", hostname);

  Ntp * ntp = (Ntp *) arg;
  if (ipaddr)
  {
    ntp->setServerAddress(ipaddr);
    print(log_debug, "NTP server address %s", "", ipaddr_ntoa(ipaddr));
    ntp->request();
  }
  else
  {
    // TODO exception
    print(log_error, "NTP DNS request failed", "");
    ntp->result(-1, NULL);
  }
}

// NTP data received
static void ntp_recv(void *arg, struct udp_pcb *pcb, struct pbuf *p, const ip_addr_t *addr, u16_t port)
{
  print(log_debug, "Received data from NTP Server", "");

  Ntp * ntp = (Ntp *) arg;
  uint8_t mode = pbuf_get_at(p, 0) & 0x7;
  uint8_t stratum = pbuf_get_at(p, 1);

  // Check the result
  if (ip_addr_cmp(addr, ntp->getServerAddress()) && port == NTP_PORT && p->tot_len == NTP_MSG_LEN &&
      mode == 0x4 && stratum != 0)
  {
    uint8_t seconds_buf[4] = {0};
    pbuf_copy_partial(p, seconds_buf, sizeof(seconds_buf), 40);
    uint32_t seconds_since_1900 = seconds_buf[0] << 24 | seconds_buf[1] << 16 | seconds_buf[2] << 8 | seconds_buf[3];
    uint32_t seconds_since_1970 = seconds_since_1900 - NTP_DELTA;
    time_t epoch = seconds_since_1970;
    ntp->result(0, &epoch);
  }
  else
  {
    // TODO exception
    print(log_error, "Invalid NTP response", "");
    ntp->result(-1, NULL);
  }
}

