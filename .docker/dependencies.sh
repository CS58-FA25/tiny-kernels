#!/bin/sh

dpkg --add-architecture i386
apt update

DEBIAN_FRONTEND=noninteractive apt install --yes \
	gcc \
	gcc-multilib \
	libc6-dev-i386 \
	make \
	libelf-dev:i386 \
	libc6-dev \
