#!/bin/bash

set -euxo pipefail

readonly BIN="$(dirname "$(readlink -f "$0")")"
readonly TARGET="${BIN}/build"
readonly SRC="/var/tmp/nginx-src"

mkdir -p "${TARGET}"
cp -r /usr/share/nginx/src/ "${SRC}"
pushd "${SRC}"
time ./configure --with-cc-opt='-g -O2 -Wmissing-prototypes -Wmissing-declarations -ffile-prefix-map=/build/nginx-kXOr6i/nginx-1.24.0=. -fstack-protector-strong -Wformat -Werror=format-security -fPIC -Wdate-time -D_FORTIFY_SOURCE=2 ' --with-ld-opt='-Wl,-z,relro -Wl,-z,now -fPIC' --prefix=/usr/share/nginx --conf-path=/etc/nginx/nginx.conf --http-log-path=/var/log/nginx/access.log --error-log-path=stderr --lock-path=/var/lock/nginx.lock --pid-path=/run/nginx.pid --modules-path=/usr/lib/nginx/modules --http-client-body-temp-path=/var/lib/nginx/body --http-fastcgi-temp-path=/var/lib/nginx/fastcgi --http-proxy-temp-path=/var/lib/nginx/proxy --http-scgi-temp-path=/var/lib/nginx/scgi --http-uwsgi-temp-path=/var/lib/nginx/uwsgi --with-compat --with-debug --with-pcre-jit --add-dynamic-module="${BIN}"
# cd objs
time make -f objs/Makefile -j 4 modules
[ -d "${TARGET}" ] || mkdir -p "${TARGET}"
cp objs/ngx_http_passthru_module.so "${TARGET}"
popd
rm -rf "${SRC}"

echo "load_module modules/ngx_http_passthru_module.so;" > "${TARGET}/mod-http-passthru-module.conf"
