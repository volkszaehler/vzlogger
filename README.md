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

If you're impatient you can quickstart using (Debian Wheezy):

    sudo apt-get install build-essential git-core cmake pkg-config subversion libcurl3-dev \
      libgnutls-dev libsasl2-dev uuid-dev uuid-runtime libtool dh-autoreconf libunistring-dev

For Debian Jessie, be sure to add:

    sudo apt-get install libgcrypt20-dev

For Debian Stretch, use:

    sudo apt-get install git cmake autoconf libtool uuid-dev libcurl4-openssl-dev libssl-dev \
      libgnutls28-dev libgcrypt20-dev libmicrohttpd-dev libsasl2-dev libunistring-dev
      
For Debian Buster, use:

    sudo apt-get install git cmake autoconf libtool uuid-dev libcurl4-openssl-dev libssl-dev \
      libgnutls28-dev libgcrypt20-dev libmicrohttpd-dev libsasl2-dev libunistring-dev build-essential

Ubuntu 18.04 LTS (bionic) needs an additional:

    sudo apt-get install libunistring-dev
(this might be needed on others now as well as we link unconditionally against libunistring)

If you want to use MQTT support:

    sudo apt-get install libmosquitto-dev

Then run the installation:

    wget --no-check-certificate https://raw.github.com/volkszaehler/vzlogger/master/install.sh
    sudo bash install.sh

Mailing List
-------------
If you have questions, contact the volkszaehler mailing lists:

  * Users mailing list: https://demo.volkszaehler.org/mailman/listinfo/volkszaehler-users
  * Developers mailing list: https://demo.volkszaehler.org/mailman/listinfo/volkszaehler-dev

More information is available in our wiki:
http://wiki.volkszaehler.org/software/controller/vzlogger
