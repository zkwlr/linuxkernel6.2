#!/bin/bash

cp /boot/config-$(uname -r) ./.config
make menuconfig
scripts/config --disable SYSTEM_TRUSTED_KEYS
scripts/config --disable SYSTEM_REVOCATION_KEYS
scripts/config --disable DEBUG_INFO
scripts/config --enable DEBUG_INFO_NONE
