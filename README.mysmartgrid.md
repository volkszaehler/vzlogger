MySmartGrid API
==================
**vzlogger** can also send measurement values to a MySmartGrid webserver.
The channel-configuration has some MySmartGrid specific parameters

To generate the Unique Ids one can use the command `uuidgen`

General channel parameters:
---------------------------

Set `"api"` to`"mysmartgrid"` to use the MySmartGrid API.

`"middleware"` defines the MySmartGrid server url.

`"uuid"`       defines the Unique ID of the sensor.

example:
````
    "api"        : "mysmartgrid",
    "middleware" : "https://api.mysmartgrid.de:8443",
    "uuid"       : "e0c9dd9a-8f3c-4fc4-8a99-f2393ca60924",
````
Mysmartgrid channel parameters
-------------------------------

`"type"`      should be `"sensor"` to send the sensor data
`"device"`    unique id for the server to identifiy the device/vzlogger 
`"secretKey"` unique secret key to sign the messages to the server

`"name"` gives the functionname of the sensor

```"channels" : [{...}[,{...}]]```

for each channel define:


    "type"       : "sensor",

    "device"     : "52709939-4183-472e-a3bb-78d944da9937",
    "secretKey"  : "57c56da4-5932-497c-bd80-6cb210c0b891",
    "name"       : "s01",

    "interval" : 300, /* */
    /* identifier for measurement: 1-0:1.8.0 */
    "identifier" : "counter",
    "scaler" : 1,  /* sml counter is in Wh */
    // "scaler" : 1000,  /* d0 counter is in kWh */

