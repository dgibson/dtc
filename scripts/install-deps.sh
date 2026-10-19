#!/bin/sh
# SPDX-License-Identifier: GPL-2.0-or-later

set -e

if [ "$(uname -s)" = "Darwin" ]
then
    brew install --quiet libyaml swig meson ninja pkg-config
    exit 0
fi

if [ "$(uname -s)" = "FreeBSD" ]
then
    pkg install -y git gmake flex bison meson ninja \
    python3 py312-setuptools swig libyaml pkgconf
    exit 0
fi

if [ -f /etc/os-release ]
then
    . /etc/os-release
else
    echo "ERROR: OS name is not provided."
    exit 1
fi

if [ "$NAME" = "Arch Linux" ]
then
    pacman -Syu --needed --noconfirm bison diffutils flex gcc git libyaml \
    make meson pkgconf python python-setuptools-scm swig
elif [ "$NAME" = "Alpine Linux" ]
then
    apk add build-base bison flex git yaml yaml-dev python3-dev \
    meson py3-setuptools_scm swig
elif [ "$NAME" = "Fedora Linux" ]
then
    dnf install -y bison diffutils flex gcc git libyaml libyaml-devel \
    make meson python3-devel python3-setuptools swig
elif [ "$NAME" = "Ubuntu" ]
then
    apt update
    apt install -yq build-essential bison flex git libyaml-dev pkg-config \
    meson python3-dev python3-setuptools python3-setuptools-scm swig
else
    echo "ERROR: OS name is not provided."
    exit 1
fi
