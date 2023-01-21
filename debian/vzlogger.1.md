# NAME

vzlogger -- A tool to read and log measurements of a wide variety of smart meters and sensors 

# SYNOPSIS

**vzlogger** [**-c** *conf*] [**-o** *log*] [**-f**] [**-l**] [**-p** *port*] [**-r**] [**-v**] [**-h**] [**-V**]


# DESCRIPTION

This is the man page for the vzlogger program. It briefly documents
the parameters. The main documentation is available online. A list 
of supported protocols can be obtained with the -h option.


**-c, --config** **<file>**
:     This option sets the configuration file to read from. The default is
**/etc/vzlogger.conf**. Note that command line options override configuration
file entries.

**-o, --log** **<file>**
:     This options sets the file to log output to. 

**-f, --foreground**
:     This prevents detaching from the terminal and lets the program run
in the foreground.

**-l, --httpd**
:     This activates a local interface tiny HTTPd which serves live readings.

**-p, --httpd-port**
:     This sets the TCP port the HTTPd is listening on.

**-r, --register**
:     This registers a device.

**-v, --verbose**
:     This enables verbose output.

**-h, --help**
:     This shows the help.

**-V, --version**
:     This shows the version of vzlogger.


# COPYRIGHT
The vzlogger program is Copyright (C) by Steffen Vogel <stv0g@0l.de> and others.

# SEE ALSO
[vzlogger.conf parameters](https://wiki.volkszaehler.org/software/controller/vzlogger/vzlogger_conf_parameter?s[]=vzlogger&s[]=conf)

