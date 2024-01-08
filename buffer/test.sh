#!/bin/bash

set -euxo pipefail

readonly BIN="$(dirname "$(readlink -f "$0")")"

"${BIN}/../common/test.sh"
