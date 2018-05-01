#!/bin/bash

set -e

CONTAINER=pdbg-build

Dockerfile=$(cat << EOF
FROM ubuntu:18.04
RUN apt-get update && apt-get install --no-install-recommends -yy \
	make \
	gcc-arm-linux-gnueabi \
	libc-dev-armel-cross \
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

# Out-of-tree build
# TODO: clean up when the build fails
SRCDIR=$PWD
TEMPDIR=`mktemp -d ${HOME}/pdbgobjXXXXXX`
RUN_TMP="docker run --rm=true --user=${USER} -w ${TEMPDIR} -v ${HOME}:${HOME} -t ${CONTAINER}"
${RUN_TMP} ${SRCDIR}/configure --host=arm-linux-gnueabi
${RUN_TMP} make
rm -rf ${TEMPDIR}

# In-tree build
${RUN} ./configure --host=arm-linux-gnueabi
${RUN} make
