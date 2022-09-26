vzlogger
=========
  * is a part of the volkszaehler.org smart meter
  * is a tool to read and log measurements of a wide variety of smart meters and sensors to the volkszaehler.org middleware
  * can run as a daemon or via cron
  * includes a tiny onboard http daemon to serve realtime readings
  * is written in C++ and should run on most embedded devices which conform to the POSIX standard

Feel free to implement support your own hardware ;)

Installation
---------------
To install, follow the detailed installation instructions at http://wiki.volkszaehler.org/software/controller/vzlogger/installation_cpp-version

If you're impatient you can quickstart using (Debian Bullseye or Ubuntu 18.04 LTS):

    sudo apt-get install build-essential git-core cmake pkg-config subversion libcurl3-dev \
      libgnutls-dev libsasl2-dev uuid-dev uuid-runtime libtool dh-autoreconf libunistring-dev

If you want to use MQTT support:

    sudo apt-get install libmosquitto-dev

Then run the installation:

    wget https://raw.github.com/volkszaehler/vzlogger/master/install.sh
    sudo bash install.sh
    
Docker
------

You can also build a docker image:

     docker build -t vzlogger .
     
Note, that this will use the newest vzlogger from volkszaehler github (not your local clone).
You can start it:

     docker run --restart=always -v /home/pi/projects/vzlogger-docker:/cfg \
     --device=/dev/serial/by-id/usb-FTDI_FT230X_Basic_UART_D30A9U5N-if00-port0 \
     --name vzlogger -d vzlogger

where /home/pi/projects/vzlogger-docker is the path to the directory containing the vzlogger.conf file and
/dev/serial/by-id/usb-FTDI_FT230X_Basic_UART_D30A8U6N-if00-port0 is your device. You can pass several devices if you have them.

Mailing List
-------------
If you have questions, contact the volkszaehler mailing lists:

  * Users mailing list: https://demo.volkszaehler.org/mailman/listinfo/volkszaehler-users
  * Developers mailing list: https://demo.volkszaehler.org/mailman/listinfo/volkszaehler-dev

More information is available in our wiki:
http://wiki.volkszaehler.org/software/controller/vzlogger
