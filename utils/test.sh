#!/bin/bash

set -e

CONTAINER=pdbg-check

Dockerfile=$(cat << EOF
FROM ubuntu:18.04
RUN apt-get update && apt-get install --no-install-recommends -yy \
	make \
	gcc \
	autoconf \
	automake \
	libtool \
	git \
	device-tree-compiler
RUN groupadd -g ${GROUPS} ${USER} && useradd -d ${HOME} -m -u ${UID} -g ${GROUPS} ${USER}
USER ${USER}
ENV HOME ${HOME}
RUN /bin/bash
EOF
)

docker pull ubuntu:18.04
docker build -t ${CONTAINER} - <<< "${Dockerfile}"

RUN="docker run --rm=true --user=${USER} -w ${PWD} -v ${HOME}:${HOME} -t ${CONTAINER}"

${RUN} ./bootstrap.sh
${RUN} ./configure
${RUN} make clean
${RUN} make
${RUN} make check
