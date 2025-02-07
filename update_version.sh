#!/bin/bash

version_number=107
current_date=$(date +%Y%m%d)

hex_version=$(printf "%08X" $version_number)

echo "New version number: $version_number"
echo "HEX: 0x$hex_version"
echo "build date: $current_date"

sed -i "s/\(#define FIRMWARE_VERSION\)[[:space:]]\+[0x0-9A-Fa-f]\+/\1 0x$hex_version/" main/common.h
sed -i 's/#define FW_BUILD_DATE[[:space:]]*"[^"]*"/#define FW_BUILD_DATE "'"$current_date"'"/' main/common.h
grep "#define FW_BUILD_DATE" main/common.h