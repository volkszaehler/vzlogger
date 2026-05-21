# Dockerfile to build a buildroot-based cross-compile environment for vzlogger

# the OS version used here is largely irrelevant,
# as buildroot compiles it's own toolchain
ARG	DEBIAN_VERSION=buster
FROM	buildpack-deps:$DEBIAN_VERSION as builder

LABEL	maintainer="Thorben T. <r00t@constancy.org>"

# don't inherit upstream labels
LABEL	org.label-schema.build-date=null
LABEL	org.label-schema.license=null
LABEL	org.label-schema.name=null
LABEL	org.label-schema.schema-version=null
LABEL	org.label-schema.vendor=null

# install host-OS packages required by buildroot
# (buildpack-deps already contains all but bc, cpio and rsync)
RUN	set -xe ; \
	sed -i '/^deb/s/ / [trusted=yes] /' /etc/apt/sources.list ; \
	apt-get update ; \
	apt-get -y --force-yes install \
		# https://buildroot.org/downloads/manual/manual.html#requirement
		#make gcc g++ \
		#patch \
		#bzip2 \
		cpio \
		#unzip \
		rsync \
		#file \
		bc \
		#libncurses-dev \
		#wget \
		#git \
		# buildroot will build it's own cmake because it's newer than debian's
		##cmake \
	; \
	apt-get purge ; \
	rm -fr /var/lib/apt/lists

WORKDIR	/buildroot

RUN \
	set -xe ; \
	\
	# download and unpack buildroot
	wget --progress=dot:mega https://buildroot.org/downloads/buildroot-2021.02.1.tar.bz2 ; \
	tar xj --strip-components=1 <buildroot-2021.02.1.tar.bz2 ; \
	rm buildroot-2021.02.1.tar.bz2

# install a helper for neatly formatted build output
RUN \
	set -xe ; \
	( \
	echo '#!/bin/bash' ;\
	echo 'f=/tmp/br_make_wrapper.$$' ; \
	echo 'trap "rm -f $f" EXIT' ; \
	echo 'echo "=== make $@ ===" >&2' ; \
	echo 'make "$@" &> >(tee "$f" | grep --line-buffered ">>>") || {' ; \
        echo '    e=$?' ; \
        echo '    cat "$f" >&2' ; \
        echo '    exit $e' ; \
	echo '}' ; \
	) >br_make_wrapper ; \
	chmod +x br_make_wrapper

ARG	DEFCONFIG=raspberrypi
# create a minimal buildroot system configuration
RUN \
	set -xe ; \
	grep ^BR2_ "configs/${DEFCONFIG}_defconfig" | grep -vE '^BR2_(LINUX|TOOLCHAIN|SYSTEM|PACKAGE|TARGET|ROOTFS)' >.config ; \
	# WCHAR support is needed at least for libunistring
	echo BR2_TOOLCHAIN_BUILDROOT_WCHAR=y >>.config ; \
	# we can disable these as we never build an OS image
	echo BR2_INIT_NONE=y >>.config ; \
	echo BR2_SYSTEM_BIN_SH_NONE=y >>.config ; \
	echo BR2_TOOLCHAIN_BUILDROOT_CXX=y >>.config ; \
	echo "# BR2_PACKAGE_BUSYBOX is not set" >>.config ; \
	echo "# BR2_PACKAGE_IFUPDOWN_SCRIPTS is not set" >>.config ; \
	echo "# BR2_PACKAGE_HOST_PATCHELF is not set" >>.config ; \
	echo "# BR2_TARGET_ROOTFS_TAR is not set" >>.config ; \
	\
	# calculate full config based on above settings + defaults
	make olddefconfig

# download required sources
RUN	./br_make_wrapper source

# build toolchain
RUN	./br_make_wrapper toolchain

# WIP: we could package the toolchain, but seems pointless with docker
#RUN	./br_make_wrapper sdk
#RUN	ls -l buildroot/output/images/*_sdk-buildroot.tar.gz

RUN \
	set -xe ; \
	# create libsml package configuration
	mkdir -p package/libsml ; \
	( \
		echo 'config BR2_PACKAGE_LIBSML' ; \
		echo 'bool "libsml"' ; \
		echo 'help' ; \
		echo '    libsml' ; \
		echo 'select BR2_PACKAGE_UTIL_LINUX' ; \
		echo 'select BR2_PACKAGE_UTIL_LINUX_LIBUUID' ; \
	) > package/libsml/Config.in ; \
	( \
		echo 'LIBSML_SITE = https://github.com/volkszaehler/libsml.git' ; \
		echo 'LIBSML_SITE_METHOD = git' ; \
		echo 'LIBSML_GIT_SUBMODULES = NO' ; \
		# this is a hack, it only works if we delete the cache in dl/vzlogger/git before re-building,
		echo 'LIBSML_VERSION = origin/master' ; \
		echo 'LIBSML_DEPENDENCIES = util-linux' ; \
		echo 'LIBSML_BUILD_CMDS = make -C $(@D)/sml CC=$(TARGET_CC) LD=$(TARGET_LD) AR=$(TARGET_AR)' ; \
		echo 'LIBSML_INSTALL_STAGING = YES' ; \
		echo 'LIBSML_INSTALL_STAGING_CMDS = make -C $(@D)/sml install DESTDIR=$(STAGING_DIR)' ; \
		echo '$(eval $(generic-package))' ; \
	) > package/libsml/libsml.mk ; \
	\
	# create vzlogger package configuration
	mkdir -p package/vzlogger ; \
	( \
		echo 'config BR2_PACKAGE_VZLOGGER' ; \
		echo 'bool "vzlogger"' ; \
		echo 'help' ; \
		echo '    vzlogger' ; \
		# have the package pull in vzlogger's deps
		echo 'select BR2_TOOLCHAIN_BUILDROOT_CXX' ; \
		echo 'select BR2_PACKAGE_LIBSML' ; \
		echo 'select BR2_PACKAGE_JSON_C' ; \
		echo 'select BR2_PACKAGE_LIBMICROHTTPD' ; \
		echo 'select BR2_PACKAGE_LIBUNISTRING' ; \
		echo 'select BR2_PACKAGE_UTIL_LINUX' ; \
		echo 'select BR2_PACKAGE_UTIL_LINUX_LIBUUID' ; \
		echo 'select BR2_PACKAGE_OPENSSL' ; \
		echo 'select BR2_PACKAGE_LIBGCRYPT' ; \
		echo 'select BR2_PACKAGE_LIBOPENSSL' ; \
		echo 'select BR2_PACKAGE_LIBCURL' ; \
		echo 'select BR2_PACKAGE_LIBCURL_OPENSSL' ; \
	) > package/vzlogger/Config.in ; \
	\
	( \
		echo 'VZLOGGER_SITE = https://github.com/volkszaehler/vzlogger.git' ; \
		echo 'VZLOGGER_SITE_METHOD = git' ; \
		echo 'VZLOGGER_GIT_SUBMODULES = NO' ; \
		# this is a hack, it only works if we delete the cache in dl/vzlogger/git before re-building,
		echo 'VZLOGGER_VERSION = origin/master' ; \
		echo 'VZLOGGER_DEPENDENCIES = libgcrypt libunistring json-c libmicrohttpd util-linux libcurl libsml' ; \
		echo 'VZLOGGER_CONF_OPTS = -DENABLE_OMS=OFF -DBUILD_TEST=ON' ; \
		echo '$(eval $(cmake-package))' ; \
	) >package/vzlogger/vzlogger.mk ; \
	\
	# add our vzlogger package to buildroot
	sed -i '1a        source "package/libsml/Config.in"' package/Config.in ; \
	sed -i '1a        source "package/vzlogger/Config.in"' package/Config.in ; \
	\
	# enable our vzlogger package
	echo BR2_PACKAGE_VZLOGGER=y >>.config ; \
	\
	# calculate full config based on above settings + defaults
	make olddefconfig ; \
	\
	# sanity check to ensure config was applied correcly
	grep -q BR2_PACKAGE_VZLOGGER=y .config ; \
	grep -q BR2_PACKAGE_LIBSML=y .config ; \
	grep -q BR2_TOOLCHAIN_BUILDROOT_WCHAR=y .config

# build deps of vzlogger
# (this will also build libsml and download vzlogger source, which we delete again)
RUN	./br_make_wrapper vzlogger-depends && rm -fr dl/vzlogger output/build/vzlogger-* dl/libsml output/build/libsml-* output/host/*/sysroot/usr/local/lib/libsml.*

# this stuff is not needed (anymore)
# (but we need to keep the .stamp files inside the build directories,
#  so buildroot remembers it has already installed the packages)
RUN	rm -fr \
		dl \
		output/build/*/* \
		output/host/share/{doc,man} \
		output/target/usr/{share,include} \
		output/target/usr/bin/[!l]* && \
	du -ch . | sort -h | tail -n 30 ; ( du -sh / 2>/dev/null || true )

# ensure build works
#RUN \
#	set -xe ; \
#	./br_make_wrapper vzlogger-build ;\
#	file output/build/vzlogger-*/src/vzlogger ; \
#	ls -l output/build/vzlogger-*/src/vzlogger ; \
#	rm -fr dl/vzlogger output/build/vzlogger-*

# use the base debian version, not buildpack-deps
# (we need to install rsync anyway, to make buildroot happy,
#  and for the final image we don't need much of buildpack-deps.)
FROM	debian:$DEBIAN_VERSION as output

# or use buildpack-deps for the final image, because we already have it anyway?
# (we COULD hack buildroot to work without rsync.)
#FROM buildpack-deps:$DEBIAN_VERSION as output

COPY	--from=builder /buildroot /buildroot

WORKDIR	/buildroot

# disable buildroot's dependency check
# we install a minimal set of host-software that works for building vzlogger,
# other buildroot operations will be broken, but we don't need them
RUN	ln -sf /bin/true support/dependencies/dependencies.sh

# docker-COPY-ing the buildroot tree clobbers some timestamp,
# making buildroot want to run `make syncconfig`, which would require gcc.
# this prevents this
RUN	touch output/build/buildroot-config/auto.conf

# buildroot thinks it needs this, but we know we don't
# (it was pre-installed on the builder, so it did not build it's own yet)
# (the order / relative timing of these really does matter.)
RUN    mkdir -p output/build/host-xz-5.2.5 && for s in downloaded extracted patched configured built installed host_installed ; do touch output/build/host-xz-5.2.5/.stamp_$s ; done

RUN	set -xe ; \
	apt-get update ; \
	apt-get -y --force-yes install \
		# these are required to run a vzlogger build
		make \
		rsync \
		git \
		#cmake \
		# only for showing the type of the binary below
		file \
	; \
	apt-get purge ; \
	rm -fr /var/lib/apt/lists

FROM output as test
# ensure build works (also usage example for final image)
RUN \
	set -xe ; \
	./br_make_wrapper vzlogger-build ;\
	file output/build/vzlogger-*/src/vzlogger ; \
	ls -l output/build/vzlogger-*/src/vzlogger ; \
	file output/build/vzlogger-origin_master/tests/vzlogger_unit_tests ; \
	ls -l output/build/vzlogger-origin_master/tests/vzlogger_unit_tests

FROM output
