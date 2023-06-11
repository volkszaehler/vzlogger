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

Debian Packages
-------------

[![Hosted By: Cloudsmith](https://img.shields.io/badge/OSS%20hosting%20by-cloudsmith-blue?logo=cloudsmith&style=flat-square)](https://cloudsmith.com)

We now build debian packages for amd64, armhf and arm64 as part of our releases.
The ones attached to the release are meant for trixie. These and packages for 
bookworm and bullseye are also provided through a repository graciously provided by 
[Cloudsmith](https://cloudsmith.com).

Those debian packages are built with MQTT support.

The setup of the repository is also 
[explained by Cloudsmith](https://cloudsmith.io/~volkszaehler/repos/volkszaehler-org-project/setup/#formats-deb).
This boils down to adding a file to /etc/apt/sources.list.d/ containing
```
deb [signed-by=/usr/share/keyrings/volkszaehler-volkszaehler-org-project-archive-keyring.gpg] https://dl.cloudsmith.io/public/volkszaehler/volkszaehler-org-project/deb/debian bookworm main
deb-src [signed-by=/usr/share/keyrings/volkszaehler-volkszaehler-org-project-archive-keyring.gpg] https://dl.cloudsmith.io/public/volkszaehler/volkszaehler-org-project/deb/debian bookworm main
```
(replace bookworm with your current distro) and retrieving the key as a trusted one
```
curl -1sLf "https://dl.cloudsmith.io/public/volkszaehler/volkszaehler-org-project/gpg.21DBDAC56DF44DA1.key" | \
	gpg --dearmor > /usr/share/keyrings/volkszaehler-volkszaehler-org-project-archive-keyring.gpg
```

Mailing List
-------------
If you have questions, contact the volkszaehler mailing lists:

  * Users mailing list: https://demo.volkszaehler.org/mailman/listinfo/volkszaehler-users
  * Developers mailing list: https://demo.volkszaehler.org/mailman/listinfo/volkszaehler-dev

More information is available in our wiki:
http://wiki.volkszaehler.org/software/controller/vzlogger
