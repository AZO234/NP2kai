#!/usr/bin/bash

# charactor code convert to UTF-8 script for NP2

# If compare two different NP2 sesies code,
# use this script each other.

# This script is for maintenance.
# Don't use to release !

set -f

FIND_EXTS=("txt" "c" "h" "cpp" "hpp" "str" "res" "inc" "tbl" "mcr" "x86" "x64" "pch" "vcproj" "filters" "sln" "xml" "am" "ac")
FIND_CMD_HEAD="find ."
FIND_CMD_LAST=" -type f"
FIND_CMD=${FIND_CMD_HEAD}

FIND_CMD="${FIND_CMD} -name *.${FIND_EXTS}"
unset FIND_EXTS[0]
FIND_EXTS=(${FIND_EXTS[@]})
for EXT in ${FIND_EXTS[@]}; do FIND_CMD="${FIND_CMD} -or -name *.${EXT}"; done
FIND_CMD="${FIND_CMD} ${FIND_CMD_LAST}"
echo ${FIND_CMD}
FILES=`${FIND_CMD}`

# UTF-8 + Yen->BackSlash
for f in ${FILES}
do
echo "${f}"
nkf --overwrite -w "${f}"
sed -i -e 's/¥/\\/g' "${f}"
done

set +f

