#!/bin/bash

CONTAINER=pdbg-build

Dockerfile=$(cat << EOF
FROM ubuntu:15.10
RUN apt-get update && apt-get install --no-install-recommends -yy \
	make \
	gcc-arm-linux-gnueabi \
	libc-dev-armel-cross \
	autoconf \
	automake
RUN groupadd -g ${GROUPS} ${USER} && useradd -d ${HOME} -m -u ${UID} -g ${GROUPS} ${USER}
USER ${USER}
ENV HOME ${HOME}
RUN /bin/bash
EOF
)

docker pull ubuntu:15.10
docker build -t ${CONTAINER} - <<< "${Dockerfile}"


docker run --rm=true --user="${USER}" \
 -w "${PWD}" -v "${HOME}":"${HOME}" -t ${CONTAINER} \
 ./bootstrap.sh 

docker run --rm=true --user="${USER}" \
 -w "${PWD}" -v "${HOME}":"${HOME}" -t ${CONTAINER} \
 ./configure --host=arm-linux-gnueabi

docker run --rm=true --user="${USER}" \
 -w "${PWD}" -v "${HOME}":"${HOME}" -t ${CONTAINER} \
 make
