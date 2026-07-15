#!/bin/sh
set -eu

CROSSOVER_BOTTLE=${CROSSOVER_BOTTLE:-Tigule}
export CROSSOVER_BOTTLE

exec "$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)/wine.sh" "$@"
