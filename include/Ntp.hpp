
class Ntp
{
  public:
    Ntp();
    ~Ntp();

    time_t queryTime();
    void request();
    void result(int status, time_t *result);
    void setServerAddress(const ip_addr_t * ipaddr);
    const ip_addr_t * getServerAddress();

  private:
    ip_addr_t        ntp_server_address;
    bool             dns_request_sent;
    struct udp_pcb * ntp_pcb;
    absolute_time_t  ntp_test_time;
    alarm_id_t       ntp_resend_alarm;
    time_t           theTime;
};

