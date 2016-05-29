#!/bin/bash
#
# Installer
#
# @copyright Copyright (c) 2015, The volkszaehler.org project
# @license http://www.opensource.org/licenses/gpl-license.php GNU Public License
# @author Andreas Goetz
#
##
# The installer will clone all required repositories or update them if necessary.
# Then the modules are compiled and installed
#
# USAGE:
#
#   Run install.sh from vzlogger or parent folder
#
#   	./install.sh
#
#   To execute specific parts of the build select which ones to run:
#
#   	./install.sh <list of modules>
#
#   Modules:
#     - vzlogger (libraries must be in place already)
#     - libsmljson
#     - libsml
#     - clean (will clean the respektive make targets, requires explicitly naming the modules)
#
# 	To run a clean build:
#
#   	./install.sh vzlogger libjson libsml clean
#
##
# This file is part of volkzaehler.org
#
# volkzaehler.org is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# any later version.
#
# volkzaehler.org is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with volkszaehler.org. If not, see <http://www.gnu.org/licenses/>.
##

set -e
shopt -s nocasematch

###############################
# some defaults
lib_dir=libs
vzlogger_conf=/etc/vzlogger.conf
git_config=.git/config

###############################
# functions
ask() {
	question="$1"
	default="$2"
	read -e -p "$question [$default] "
	REPLY="${REPLY:-$default}"
}

contains() {
    [[ $1 =~ $2 ]] && true || false
}

git_is_repo() {
	folder="$1"
	match="${2-$folder}"

	if [ -e "$folder/$git_config" ] && grep -q "$match" "$folder/$git_config"; then
		true
	else
		false
	fi
}

git_update() {
	folder="$1"
	git_repo="$2"
	match="${3-$folder}"
	git_params="${4-}"

	if git_is_repo $folder $match; then
		echo "$folder folder: $(pwd)/$folder"
		git_dirty=$(cd $folder; git fetch; git log HEAD.. --oneline)
	else
		echo "$folder not found"
		echo "cloning $folder git repository"
		git clone $git_params "$git_repo" $folder
	fi

	if [ -n "$git_dirty" ]; then
		echo "updating $folder git repository with remote changes"
		pushd $folder
			git pull
		popd
	fi
}


###############################
# header
echo "vzlogger installation script"

###############################
# check prerequisites
echo
echo -n "checking prerequisites:"

deps=( grep pidof git cmake pkg-config autoreconf )
for binary in "${deps[@]}"; do
	if binpath="$(which $binary)" ; then
		echo -n " $binary"
	else
		echo
		echo " $binary: not found. Please install to use this script (e.g. sudo apt-get install $binary)."
		exit 1
	fi
done
echo

###############################
echo
echo "vzlogger setup..."
if [ -n "$1" ]; then
	echo "setup modules: $1"
fi

###############################
echo
echo "checking for vzlogger folder"

if ! git_is_repo . vzlogger ; then
	# move to parent folder

	if git_is_repo vzlogger ; then
		cd vzlogger
	else
		if [ -d vzlogger ]; then
			echo "directory ./vzlogger does not contain a recognized git repo"
		fi

		mkdir vzlogger
		cd vzlogger
	fi
fi

if [ -z "$1" ] || contains "$*" vzlogger; then
	git_update . https://github.com/volkszaehler/vzlogger.git vzlogger
fi


###############################
echo
echo "checking for libraries"

if [ ! -d "$lib_dir" ]; then
	echo "creating library folder $lib_dir"
	mkdir "$lib_dir"
fi
pushd "$lib_dir"

	###############################
	# libjson
	if [ -z "$1" ] || contains "$*" libjson; then
		echo
		echo "checking for libjson"

		git_update json-c https://github.com/json-c/json-c json-c "-b json-c-0.12"
	fi

	# libsml
	if [ -z "$1" ] || contains "$*" libsml; then
		echo
		echo "checking for libsml"

		git_update libsml https://github.com/volkszaehler/libsml.git
	fi

	# libmbus
	if [ -z "$1" ] || contains "$*" libmbus; then
		echo
		echo "checking for libmbus"

		git_update libmbus https://github.com/rscada/libmbus.git
	fi


	###############################
	echo
	echo "building and installing libraries"

	# libjson
	if [ -z "$1" ] || contains "$*" libjson; then
		echo
		echo "building and installing libjson"
		pushd json-c
			if [ ! -e Makefile ]; then
				sh autogen.sh
				./configure
			else
				if contains "$*" clean; then make clean; fi
			fi

			make
			sudo make install
		popd
	fi

	# libsml
	if [ -z "$1" ] || contains "$*" libsml; then
		echo
		echo "building and installing libsml"
		pushd libsml
			if contains "$*" clean; then make clean; fi

			make
			sudo cp sml/lib/libsml.* /usr/lib/
			sudo cp -R sml/include/* /usr/include/
			sudo cp sml.pc /usr/lib/pkgconfig/
		popd
	fi

	# libmbus
	if [ -z "$1" ] || contains "$*" libmbus; then
		echo
		echo "building and installing libmbus"
		pushd libmbus
			if contains "$*" clean; then make clean; fi

			./build.sh
			sudo make install
		popd
	fi

popd

###############################
# vzlogger
if [ -z "$1" ] || contains "$*" vzlogger; then
	echo
	echo "building and installing vzlogger"

	if contains "$*" clean; then
		echo "clearing cmake cache"
		rm CMakeCache.txt
	fi

	echo
	echo "building vzlogger"
	cmake .
	make

	echo
	echo "installing vzlogger"
	sudo make install

	if [ ! -e "$vzlogger_conf" ]; then
		echo
		echo "could not find global config file $vzlogger_conf"
		echo "make sure to configure vzlogger before running (see etc/vzlogger.conf)"
	fi

	if [ -n "$(pidof vzlogger)" ]; then
		echo
		echo "vzlogger is already running"
		echo "make sure to restart vzlogger"
	fi

	if ! grep -q vzlogger /etc/inittab; then
		echo
		echo "could not find vzlogger in /etc/inittab"
		echo "if you want vzlogger to start automatically add the following line to /etc/inittab"
		echo "vz:235:respawn:/usr/local/bin/vzlogger -d"
		echo "activate using `init q`"
	fi
fi
