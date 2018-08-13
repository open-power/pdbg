#!/bin/bash

set -e

CONTAINER=pdbg-build

Dockerfile=$(cat << EOF
FROM ubuntu:18.04
RUN apt-get update && apt-get install --no-install-recommends -yy \
	make \
	gcc-arm-linux-gnueabi \
	libc-dev-armel-cross \
	gcc-powerpc64le-linux-gnu \
	libc-dev-ppc64el-cross \
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

# Out-of-tree build, arm
# TODO: clean up when the build fails
SRCDIR=$PWD
TEMPDIR=`mktemp -d ${HOME}/pdbgobjXXXXXX`
RUN_TMP="docker run --rm=true --user=${USER} -w ${TEMPDIR} -v ${HOME}:${HOME} -t ${CONTAINER}"
${RUN_TMP} ${SRCDIR}/configure --host=arm-linux-gnueabi
${RUN_TMP} make
rm -rf ${TEMPDIR}

# In-tree build, arm
${RUN} ./configure --host=arm-linux-gnueabi
${RUN} make
${RUN} make clean

# In-tree build, powerpc64le
${RUN} ./configure --host=powerpc64le-linux-gnu
${RUN} make
${RUN} make clean

# In-tree build, amd64
# TODO: work out how to install a amd64 compiler if we are building on a eg.
# ppc machine in a way that still works when we're on amd64
${RUN} ./configure --host=x86-64-linux-gnu
${RUN} make
${RUN} make clean
