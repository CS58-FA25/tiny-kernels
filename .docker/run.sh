#!/bin/bash

docker run \
	--cap-add=SYS_PTRACE \
	--security-opt seccomp=unconfined \
  	--security-opt apparmor=unconfined \
	--volume $(pwd):/app \
	-it \
	--platform=linux/amd64 \
	yalnix \
	/bin/bash
