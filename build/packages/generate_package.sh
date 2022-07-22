#! /bin/bash

VARIANT=$1

VERSION_NO_SUFFIX="0.1.1"
VERSION_SUFFIX=""
VERSION="0.1.1"

# Using CMAKE_BINARY_DIR uses an absolute path and will break cross-vm building/download/make functionality
BUILD_DIR="../../build"

VENDOR=""
PROJECT="nftone_contracts"
DESC=""
URL=""
EMAIL=""

export BUILD_DIR
export VERSION_NO_SUFFIX
export VERSION_SUFFIX
export VERSION
export VENDOR
export PROJECT
export DESC
export URL
export EMAIL

. ./generate_tarball.sh

