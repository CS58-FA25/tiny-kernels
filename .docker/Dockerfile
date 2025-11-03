FROM amd64/ubuntu:latest

RUN dpkg --add-architecture i386
RUN apt update

ENV DEBIAN_FRONTEND=noninteractive 
RUN apt install --yes \
	gcc \
	gcc-multilib \
	libc6-dev-i386 \
	make \
	libelf-dev:i386 \
	libc6-dev \
	gdb \
	vim

WORKDIR /app
ENV YALNIX_FRAMEWORK=/app/.yalnix

ENTRYPOINT ["/bin/bash"]
