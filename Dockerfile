############################
# STEP 1 build executable binary
############################

FROM alpine:latest as builder

RUN apk add --no-cache \
    gcc \
    g++ \
    libc-dev \
    linux-headers \
    git \
    make \
    cmake \
    curl-dev \
    gnutls-dev \
    cyrus-sasl-dev \
    # for libuuid
    util-linux-dev \
    libtool \
    libgcrypt-dev \
    libmicrohttpd-dev \
    json-c-dev \
    mosquitto-dev \
    libunistring-dev \
    automake \
    autoconf \
    gtest-dev

WORKDIR /vzlogger

RUN git clone https://github.com/volkszaehler/libsml.git --depth 1 \
    && make install -C libsml/sml

RUN git clone https://github.com/rscada/libmbus.git --depth 1 \
    && cd libmbus \
    && ./build.sh \
    && make install

COPY . /vzlogger

ARG build_test=off
RUN cmake -DBUILD_TEST=${build_test} \
    && make \
    && make install \
    && if [ "$build_test" != "off" ]; then make test; fi


#############################
## STEP 2 build a small image
#############################

FROM alpine:latest

LABEL Description="vzlogger"

RUN apk add --no-cache \
    libcurl \
    gnutls \
    libsasl \
    libuuid \
    libgcrypt \
    libmicrohttpd \
    json-c \
    libatomic \
    mosquitto-libs \
    libunistring \
    libstdc++ \
    libgcc

# libsml is linked statically => no need to copy
COPY --from=builder /usr/local/bin/vzlogger /usr/local/bin/vzlogger
COPY --from=builder /usr/local/lib/libmbus.so* /usr/local/lib/

# without running a user context, no exec is possible and without the dialout group no access to usb ir reader possible
RUN adduser -S vz -G dialout

RUN vzlogger --version

USER vz
CMD ["vzlogger", "--foreground"]
