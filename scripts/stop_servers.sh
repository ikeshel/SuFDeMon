#!/usr/bin/env bash
# Author: Irakli Keshelashvili, 2026
#
# SuFDeMon - Super-FRS Detector Monitoring Software
# ROOT-based client-server monitoring for MUSIC, PLSCI and SCIFI detectors.
#
# Copyright (C) 2026 Irakli Keshelashvili
#
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, version 3.
#
# See the LICENSE file in the project root for the full license text.
#
# Close local SuFDeMon Screen sessions and the server processes inside them.
set -euo pipefail
repo_dir=$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/.." && pwd)
if [[ ${1:-} == --help || ${1:-} == -h ]]; then
    echo "Usage: $0"
    echo 'Stops all SuFDeMon-* Screen sessions owned by this user on this machine.'
    echo 'Servers started outside these sessions and remote machines are unaffected.'
    exit 0
fi
(( $# == 0 )) || { echo "Usage: $0" >&2; exit 1; }
command -v screen >/dev/null || { echo 'GNU Screen is required.' >&2; exit 1; }
command -v flock >/dev/null || { echo 'flock is required (util-linux).' >&2; exit 1; }
exec 9>"$repo_dir/.server-screen.lock"
flock -x 9
sessions=$(screen -ls 2>/dev/null || true)
mapfile -t targets < <(awk '$1 ~ /^[0-9]+\.SuFDeMon-[A-Za-z0-9_-]+$/ {print $1}' <<< "$sessions")
if (( ${#targets[@]} == 0 )); then
    echo 'No SuFDeMon server sessions are running.'
    exit 0
fi
status=0
for session in "${targets[@]}"; do
    if screen -S "$session" -X quit 9>&-; then
        echo "Stopped $session"
    else
        echo "Failed to stop $session" >&2
        status=1
    fi
done
exit "$status"
