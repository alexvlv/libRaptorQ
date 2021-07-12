#!/bin/sh

# $Id$

SRCDIR="$(cd "$(dirname "${0}")" >/dev/null 2>&1 && pwd)"
BUILDDIR=${SRCDIR}/build-mips
echo Build dir: [${BUILDDIR}]

mkdir -p ${BUILDDIR} && cd ${BUILDDIR} || exit 1


TOPDIR=/work/projects/openwrt/openwrt-18.06.9/git
export STAGING_DIR=${TOPDIR}/staging_dir
COMPILER_DIR=${STAGING_DIR}/toolchain-mips_24kc_gcc-7.3.0_musl/bin/
export PATH=${COMPILER_DIR}:${PATH}

COMPILER_PREFIX=mips-openwrt-linux-musl-

make -j 4 everything
