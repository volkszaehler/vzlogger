FROM debian:stable

LABEL Description="vzlogger"

# Dependencies 
# https://wiki.volkszaehler.org/software/controller/vzlogger/installation_cpp-version
RUN apt-get update && apt-get -y upgrade 
RUN apt-get -y install build-essential git-core cmake pkg-config \
    subversion libcurl4-openssl-dev libgnutls28-dev libsasl2-dev \
    uuid-dev libtool libssl-dev libgcrypt20-dev libmicrohttpd-dev \
    libltdl-dev libjson-c-dev libleptonica-dev libmosquitto-dev \
    libunistring-dev dh-autoreconf sudo

# Build from volkszaehler/vzlogger
# pass "N" when prompted about adding systemd unit
RUN mkdir /cfg && cd /tmp && \
    git clone https://github.com/volkszaehler/vzlogger.git && \
    cd vzlogger && \
    echo "N\n" | bash ./install.sh

# Setup volume 
VOLUME ["/cfg"]

# Run CMD when started
CMD /usr/local/bin/vzlogger --config /cfg/vzlogger.conf
