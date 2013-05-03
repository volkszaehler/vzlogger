#!/bin/bash
### BEGIN INIT INFO
# Provides:          vzlogger
# Required-Start:    $local_fs $network
# Required-Stop:     $local_fs $network
# Default-Start:     2 3 4 5
# Default-Stop:      0 1 6
# Short-Description: vzlogger Server startscript.
# Description:       Starts our vzlogger server
### END INIT INFO
# Initialize Script for Landis&Gyr D0 Meters on ttyUSB Port (udo's IR)
# (c) 2013 volkszaehler.org | Michael Wulz

#default
stty -F /dev/lesekopf0 sane

#mode einstellen
stty -F /dev/lesekopf0 300 parenb -parodd cs7 -cstopb raw -echo

#starte vzlogger
vzlogger -c /etc/vzlogger.conf
