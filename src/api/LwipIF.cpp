/**
 * Header file for volkszaehler.org API calls
 *
 * @author Kai Krueger <kai.krueger@itwm.fraunhofer.de>
 * @copyright Copyright (c) 2011, The volkszaehler.org project
 * @package vzlogger
 * @license http://opensource.org/licenses/gpl-license.php GNU Public License
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

#include <string.h>
#include <time.h>

#include "pico/stdlib.h"
#include "pico/cyw43_arch.h"
#include "lwip/pbuf.h"
#include "lwip/altcp_tcp.h"
#include "lwip/altcp_tls.h"
#include "lwip/dns.h"

#include <VZException.hpp>
#include <api/LwipIF.hpp>

#include <Config_Options.hpp>

static void  altcp_client_dns_found(const char* hostname, const ip_addr_t *ipaddr, void *arg);
static err_t altcp_client_connected(void *arg, struct altcp_pcb *pcb, err_t err);
static err_t altcp_client_poll(void *arg, struct altcp_pcb *pcb);
static void  altcp_client_err(void *arg, err_t err);
static err_t altcp_client_recv(void *arg, struct altcp_pcb *pcb, struct pbuf *p, err_t err);
static err_t altcp_client_sent(void *arg, struct altcp_pcb *pcb, u16_t len);

vz::api::LwipIF::LwipIF(uint t) : pcb(NULL), tls_config(NULL), timeout(t), state(VZ_SRV_INIT), connectInitiated(0)
{
  print(log_debug, "Creating LwipIF instance (timeout: %d)...", "", timeout);
  this->initPCB();
  print(log_debug, "Created LwipIF instance (%x).", "", pcb);
}

vz::api::LwipIF::~LwipIF()
{
  print(log_debug, "Destroying LwipIF instance (%x).", "", pcb);
  this->deletePCB();
}

void vz::api::LwipIF::initPCB()
{
  if(pcb == NULL)
  {
#ifdef VZ_USE_HTTPS
    /* No CA certificate checking */
    tls_config = altcp_tls_create_config_client(NULL, 0);
    // tls_config = altcp_tls_create_config_client(cert, cert_len);
    assert(tls_config);

    //mbedtls_ssl_conf_authmode(&tls_config->conf, MBEDTLS_SSL_VERIFY_OPTIONAL);

    pcb = altcp_tls_new(tls_config, IPADDR_TYPE_ANY);
#else // VZ_USE_HTTPS
    pcb = altcp_new_ip_type(NULL, IPADDR_TYPE_ANY);
#endif // VZ_USE_HTTPS
    if(!pcb)
    {
      throw vz::VZException("LwipIF: failed to create pcb");
    }

    altcp_arg(pcb, this);
    altcp_poll(pcb, altcp_client_poll, timeout * 2);
    altcp_recv(pcb, altcp_client_recv);
    altcp_err(pcb, altcp_client_err);
    altcp_sent(pcb, altcp_client_sent);
    print(log_debug, "Initialized PCB.", "");
  }
}

void vz::api::LwipIF::deletePCB()
{
  if(pcb != NULL)
  {
    altcp_arg(pcb, NULL);
    altcp_poll(pcb, NULL, 0);
    altcp_recv(pcb, NULL);
    altcp_err(pcb, NULL);
    altcp_sent(pcb, NULL);
    err_t err = altcp_close(pcb);
    if (err != ERR_OK)
    {
      print(log_error, "Close failed %d, calling abort\n", "", err);
      altcp_abort(pcb);
    }
    pcb = NULL;

#ifdef VZ_USE_HTTPS
    if(tls_config != NULL)
    {
      altcp_tls_free_config(tls_config);
    }
#endif // VZ_USE_HTTPS
  }
}

void vz::api::LwipIF::addHeader(const std::string value) { headers.insert(value.c_str()); }
void vz::api::LwipIF::clearHeader() { headers.clear(); }
void vz::api::LwipIF::commitHeader() { /* no-op */ }
uint vz::api::LwipIF::getPort() { return port; }
char * vz::api::LwipIF::getResponse() { return strdup(response.c_str()); }
void vz::api::LwipIF::setResponse(const char * resp) { response = resp; }

uint vz::api::LwipIF::getState() { return state; }
void vz::api::LwipIF::setState(uint state) { this->state = state; }
const char * vz::api::LwipIF::stateStr()
{
  static const char * stateStrings[] = { "initial", "connecting", "ready", "sending", "error" };
  return stateStrings[state];
}
time_t vz::api::LwipIF::getConnectInit() { return connectInitiated; }

void vz::api::LwipIF::connect(const char * h, uint p)
{
  if(p != 0) { port = p; }
  if(h != NULL) { hostname = h; }

  this->initPCB();

#ifdef VZ_USE_HTTPS
  /* Set SNI */
  mbedtls_ssl_set_hostname((mbedtls_ssl_context *) altcp_tls_context(pcb), hostname.c_str());
#endif // VZ_USE_HTTPS

  print(log_debug, "Resolving '%s'", "", hostname.c_str());

  state = VZ_SRV_CONNECTING;

  // cyw43_arch_lwip_begin/end should be used around calls into lwIP to ensure correct locking.
  // You can omit them if you are in a callback from lwIP. Note that when using pico_cyw_arch_poll
  // these calls are a no-op and can be omitted, but it is a good practice to use them in
  // case you switch the cyw43_arch type later.
  cyw43_arch_lwip_begin();

  ip_addr_t server_ip;
  err_t err = dns_gethostbyname(hostname.c_str(), &server_ip, altcp_client_dns_found, this);
  if (err != ERR_OK && err != ERR_INPROGRESS)
  {
    cyw43_arch_lwip_end();
    throw vz::VZException("LwipIF: error initiating DNS resolving, err=%d", err);
  }

  cyw43_arch_lwip_end();
  connectInitiated = time(NULL);
  print(log_debug, "DNS resolution initiated.", "");
}

struct altcp_pcb * vz::api::LwipIF::getPCB() { return pcb; }

// ==============================

uint vz::api::LwipIF::postRequest(const char * data, const char * url)
{
  if(state != VZ_SRV_READY)
  {
    print(log_debug, "Not sending request - state: %d", "", state);
    return state;
  }

  char buf[128];
  std::string request;
  sprintf(buf, "POST %s HTTP/1.1\r\n", url); request = buf;
  sprintf(buf, "Host: %s:%d\r\n", hostname.c_str(), port); request += buf;
//  request += "Connection: close\r\n";
  std::set<std::string>::iterator it;
  for (it = headers.begin(); it != headers.end(); it++)
  {
    request += (*it).c_str();
    request += "\r\n";
  }
  sprintf(buf, "Content-Length: %d\r\n", strlen(data)); request += buf;
  request += "\r\n";
  request += data;

  print(log_debug, "Sending request: %s", "", request.c_str());
  cyw43_arch_lwip_begin();
  err_t err = altcp_write(pcb, request.c_str(), strlen(request.c_str()), TCP_WRITE_FLAG_COPY);
  if (err == ERR_OK)
  {
    print(log_debug, "Flushing Lwip output ...", "");
    err = altcp_output(pcb);
  }
  cyw43_arch_lwip_end();
  if (err != ERR_OK)
  {
    throw vz::VZException("LwipIF: error sending data, err=%d", err);
  }
  print(log_debug, "Sending request complete", "");
  state = VZ_SRV_SENDING;
  return state;
}

// ==============================

static void altcp_client_dns_found(const char* hostname, const ip_addr_t *ipaddr, void *arg)
{
  if (ipaddr)
  {
    print(log_debug, "DNS resolving complete", "");

    vz::api::LwipIF * ai = (vz::api::LwipIF *) arg;
    print(log_debug, "Connecting to server IP %s port %d", "", ipaddr_ntoa(ipaddr), ai->getPort());
    err_t err = altcp_connect(ai->getPCB(), ipaddr, ai->getPort(), altcp_client_connected);
    if (err != ERR_OK)
    {
      throw vz::VZException("LwipIF: error initiating connect, err=%d", err);
    }
  }
  else
  {
    throw vz::VZException("LwipIF: error resolving hostname %s", hostname);
  }
  print(log_debug, "Connection initiated", "");
}

// ==============================

static err_t altcp_client_connected(void *arg, struct altcp_pcb *pcb, err_t err)
{
  if (err != ERR_OK)
  {
    throw vz::VZException("LwipIF: connect failed %d", err);
  }
  print(log_debug, "Server connected", "");
  vz::api::LwipIF * ai = (vz::api::LwipIF *) arg;
  ai->setState(VZ_SRV_READY);
  return ERR_OK;
}

// ==============================

static err_t altcp_client_poll(void *arg, struct altcp_pcb *pcb)
{
  vz::api::LwipIF * ai = (vz::api::LwipIF *) arg;
  if(ai->getState() == VZ_SRV_CONNECTING || ai->getState() == VZ_SRV_SENDING)
  {
    print(log_debug, "Timed out", "");
    // Maybe just keep trying ... ?? Reconnect?
    ai->setState(VZ_SRV_INIT);
  }
  return ERR_OK;
}

// ==============================

static err_t altcp_client_sent(void *arg, struct altcp_pcb *pcb, u16_t len)
{
  print(log_debug, "Sent %d bytes", "", len);
  return ERR_OK;
}

// ==============================

static void altcp_client_err(void *arg, err_t err)
{
  print(log_debug, "Error: %d", "", err);
  vz::api::LwipIF * ai = (vz::api::LwipIF *) arg;
  ai->setState(VZ_SRV_ERROR);
  throw vz::VZException("LwipIF: altcp_client_err %d", err);
}

// ==============================

static err_t altcp_client_recv(void *arg, struct altcp_pcb *pcb, struct pbuf *p, err_t err)
{
  vz::api::LwipIF * ai = (vz::api::LwipIF *) arg;
  if (!p)
  {
    print(log_debug, "Connection closed by server: %d", "", err);
    ai->setState(VZ_SRV_INIT);
    ai->deletePCB();
    return ERR_OK;
  }

  print(log_debug, "Receiving data: %d", "", err);

  if (p->tot_len > 0)
  {
    /* For simplicity this examples creates a buffer on stack the size of the data pending here, 
       and copies all the data to it in one go.
       Do be aware that the amount of data can potentially be a bit large (TLS record size can be 16 KB),
       so you may want to use a smaller fixed size buffer and copy the data to it using a loop, if memory is a concern */

    char buf[p->tot_len + 1];
    pbuf_copy_partial(p, buf, p->tot_len, 0);
    buf[p->tot_len] = 0;

    ai->setResponse(buf);
    printf("***\nnew data received from server (%d bytes):\n***\n\n%s\n", p->tot_len, buf);
    altcp_recved(pcb, p->tot_len);
  }

  pbuf_free(p);
  ai->setState(VZ_SRV_READY);
  return ERR_OK;
}

