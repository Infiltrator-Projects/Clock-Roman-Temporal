#!/bin/sh
# SPDX-License-Identifier: GPL-3.0-or-later
set -eu
command -v python3 >/dev/null 2>&1 || { echo 'Install python3 before running this source installer.' >&2; exit 1; }
work=$(mktemp -d "${TMPDIR:-/tmp}/infiltrator-provider.XXXXXXXX")
trap 'rm -rf -- "$work"' EXIT HUP INT TERM
line=$(LC_ALL=C awk '/^__INFILTRATOR_PROVIDER_SOURCE__$/ { print NR + 1; exit }' "$0")
test -n "$line"
tail -n +"$line" "$0" > "$work/source.tar.gz"
printf '%s  %s\n' '@PAYLOAD_SHA256@' "$work/source.tar.gz" | sha256sum -c - >/dev/null
tar -xzf "$work/source.tar.gz" -C "$work" --no-same-owner --no-same-permissions
python3 "$work/source/packaging/release.py" install "$@"
exit 0
__INFILTRATOR_PROVIDER_SOURCE__
