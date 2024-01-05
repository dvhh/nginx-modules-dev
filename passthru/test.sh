#!/bin/bash

set -euxo pipefail

readonly BIN="$(dirname "$(readlink -f "$0")")"

function cleanup() {
	if [ -f "${BIN}/pid" ]
	then
		/usr/sbin/nginx -c "${BIN}/test/nginx.conf" -s quit
	fi
}

trap cleanup EXIT

function fail() {
	1>&2 echo $1
	exit 1
}

# test/nginx.conf.j2
j2 -f env "${BIN}/test/nginx.conf.j2" > "${BIN}/test/nginx.conf"

/usr/sbin/nginx -t -c "${BIN}/test/nginx.conf"

/usr/sbin/nginx -c "${BIN}/test/nginx.conf"

curl -v http://localhost:5000/index.html
curl -v http://localhost:5000/proxy/index.html

readonly BIG_FILE="${BIN}/test/html/2M.bin"
[ -f "${BIG_FILE}" ] || head -c 2M /dev/random/ > "${BIG_FILE}"
md5sum "${BIG_FILE}" | tee "${BIN}/test/baseline"
curl -v http://localhost:5000/2M.bin | md5sum - | tee "${BIN}/test/normal"
curl -v http://localhost:5000/proxy/2M.bin | md5sum - | tess "${BIN}/test/proxy"

readonly BASELINE="$(cut -f 1 "${BIN}/test/baseline")"
readonly NORMAL="$(cut -f 1 "${BIN}/test/normal")"
readonly PROXY="$(cut -f 1 "${BIN}/test/proxy")"

[ "${BASELINE}" == "${NORMAL}" ] || fail "failed normal test"
[ "${BASELINE}" == "${PROXY}" ] || fail "failed proy test"
