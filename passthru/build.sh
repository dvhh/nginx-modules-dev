#!/bin/bash

set -euxo pipefail

readonly BIN="$(dirname "$(readlink -f "$0")")"

bash "${BIN}/../common/build.sh"
