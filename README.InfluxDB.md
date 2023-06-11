InfluxDB api
==================
**vzlogger** can send measurements to a InfluxDB server.

Configuration
---------------------------

Set `"api"` to`"influxdb"` to use the InfluxDB API.

`"host"` is the IP address or hostname of the InfluxDB server in the format `127.0.0.1:8086`

`"uuid"` is the unique channel ID. Use the `uuid` or `uuidgen` command to generate one.

For optional parameters such as `username` or `password` have a look at the
example config file is available at [`etc/vzlogger.conf.InfluxDB`](https://github.com/volkszaehler/vzlogger/blob/master/etc/vzlogger.conf.InfluxDB)

Message Format
---------------------------

Messages are sent in the following format:

<measurement_name>,uuid=<uuid>,<tags> value=<value> <time[ms]>

In the above, measurement_name, tags and uuid are from the configuration
while value and time[ms] are value and time of the measurement.

Details about this can be found in the [InfluxDB line protocol tutorial](https://docs.influxdata.com/influxdb/v1.8/write_protocols/line_protocol_tutorial/)
