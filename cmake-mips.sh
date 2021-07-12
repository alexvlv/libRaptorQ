#!/bin/sh

# $Id$

SRCDIR="$(cd "$(dirname "${0}")" >/dev/null 2>&1 && pwd)"
BUILDDIR=${SRCDIR}/build-mips
echo Build dir: [${BUILDDIR}]

mkdir -p ${BUILDDIR} && cd ${BUILDDIR} || exit 1

OPENWRT_DIR=/work/projects/openwrt/openwrt-18.06.9
TOPDIR=${OPENWRT_DIR}/git
export STAGING_DIR=${TOPDIR}/staging_dir
COMPILER_DIR=${STAGING_DIR}/toolchain-mips_24kc_gcc-7.3.0_musl/bin/
export PATH=${COMPILER_DIR}:${PATH}

COMPILER_PREFIX=mips-openwrt-linux-musl-

SYSROOT_DIR=${OPENWRT_DIR}/sysroot

cmake -DCMAKE_BUILD_TYPE=Release \
-DCMAKE_C_COMPILER=${COMPILER_PREFIX}gcc \
-DCMAKE_CXX_COMPILER=${COMPILER_PREFIX}g++ \
-DCMAKE_SYSROOT=${SYSROOT_DIR} \
-DCMAKE_SYSROOT_COMPILE=${SYSROOT_DIR} \
-DCMAKE_VERBOSE_MAKEFILE=ON \
../
