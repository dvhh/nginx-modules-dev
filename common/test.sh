#!/bin/bash

set -euxo pipefail

readonly BIN="$(dirname "$(readlink -f "$0")")"
readonly MODULE="${PWD}"
readonly MODULE_NAME="$(grep "ngx_addon_name=" ${MODULE}/config|sed 's/ngx_addon_name=//')"


function cleanup() {
	if [ -f "${MODULE}/pid" ]
	then
		/usr/sbin/nginx -c "${MODULE}/test/nginx.conf" -s quit
	fi
}

trap cleanup EXIT

function fail() {
	1>&2 echo $1
	exit 1
}

# test/nginx.conf.j2
export MODULE
export MODULE_NAME
env
# exit
j2 -f env "${BIN}/test/nginx.conf.j2" > "${MODULE}/test/nginx.conf"

/usr/sbin/nginx -t -c "${MODULE}/test/nginx.conf"

/usr/sbin/nginx -c "${MODULE}/test/nginx.conf"

curl -v http://localhost:5000/index.html
curl -v http://localhost:5000/proxy/index.html

readonly BIG_FILE="${MODULE}/test/html/2M.bin"
[ -f "${BIG_FILE}" ] || head -c 2M /dev/random/ > "${BIG_FILE}"
md5sum "${BIG_FILE}" | tee "${MODULE}/test/baseline"
curl -sv http://localhost:5000/2M.bin | md5sum - | tee "${MODULE}/test/normal"
curl -sv http://localhost:5000/proxy/2M.bin | md5sum - | tee "${MODULE}/test/proxy"

readonly BASELINE="$(cut -f 1 -d ' '  "${MODULE}/test/baseline")"
readonly NORMAL="$(cut -f 1 -d ' ' "${MODULE}/test/normal")"
readonly PROXY="$(cut -f 1 -d ' ' "${MODULE}/test/proxy")"

[ "${BASELINE}" == "${NORMAL}" ] || fail "failed normal test"
[ "${BASELINE}" == "${PROXY}" ] || fail "failed proy test"
