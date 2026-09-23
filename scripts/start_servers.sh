#!/usr/bin/env bash
# Start local detector servers in detached GNU Screen sessions.
set -euo pipefail
repo_dir=$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/.." && pwd)
build_dir=${BUILD_DIR:-"$repo_dir/build"}
config_dir=${CONFIG_DIR:-"$repo_dir/config/servers"}
log_dir=${LOG_DIR:-"$repo_dir/logs/servers"}

if [[ ${1:-} == --help || ${1:-} == -h ]]; then
    echo "Usage: $0 [MUSIC1 PLSCI2 SCIFI14 ...]"
    echo 'Without arguments, starts every config on this machine (no SSH).'
    echo 'Overrides: BUILD_DIR, CONFIG_DIR, LOG_DIR. Session names: SuFDeMon-<config name>.'
    exit 0
fi
command -v screen >/dev/null || { echo 'GNU Screen is required.' >&2; exit 1; }
command -v flock >/dev/null || { echo 'flock is required (util-linux).' >&2; exit 1; }
[[ -d $config_dir ]] || { echo "Missing config directory: $config_dir" >&2; exit 1; }
config_dir=$(cd -- "$config_dir" && pwd)
build_dir=$(cd -- "$build_dir" && pwd)
mkdir -p -- "$log_dir"
log_dir=$(cd -- "$log_dir" && pwd)
# Serialize start/stop operations for this checkout.
exec 9>"$repo_dir/.server-screen.lock"
flock -x 9

configs=()
if (( $# )); then
    for name in "$@"; do
        [[ $name =~ ^[A-Za-z0-9_-]+$ ]] || { echo "Invalid config name: $name" >&2; exit 1; }
        configs+=("$config_dir/$name.conf")
    done
else
    shopt -s nullglob
    configs=("$config_dir"/*.conf)
fi
(( ${#configs[@]} )) || { echo 'No server configurations found.' >&2; exit 1; }
binaries=()
# Preflight all requested files before starting any processes. Never source configs.
for config in "${configs[@]}"; do
    [[ -f $config ]] || { echo "Missing config: $config" >&2; exit 1; }
    name=$(basename -- "$config" .conf)
    [[ $name =~ ^[A-Za-z0-9_-]+$ ]] || { echo "Invalid config name: $name" >&2; exit 1; }
    type=$(awk -F= '$1 ~ /^[[:space:]]*type[[:space:]]*$/ {gsub(/[[:space:]]/, "", $2); print $2}' "$config")
    case "$type" in MUSIC|PLSCI|SCIFI) ;; *) echo "Invalid detector type in $config" >&2; exit 1;; esac
    binary="$build_dir/server/SuFDeMon${type}Server"
    [[ -x $binary ]] || { echo "Missing executable: $binary (build with make servers)" >&2; exit 1; }
    binaries+=("$binary")
done
session_exists() {
    local sessions
    sessions=$(screen -ls 2>/dev/null || true)
    awk -v wanted="$1" '$1 ~ /^[0-9]+\./ {sub(/^[0-9]+\./, "", $1); if ($1 == wanted) found=1} END {exit !found}' <<< "$sessions"
}
status=0
for i in "${!configs[@]}"; do
    name=$(basename -- "${configs[i]}" .conf)
    session="SuFDeMon-$name"
    if session_exists "$session"; then
        echo "Already running: $session"
        continue
    fi
    # Close the lock descriptor in Screen and its server child.
    if ! screen -dmS "$session" -L -Logfile "$log_dir/$name.log" "${binaries[i]}" --config "${configs[i]}" 9>&-; then
        echo "Failed to start $session" >&2
        status=1
        continue
    fi
    sleep 0.2
    if session_exists "$session"; then
        echo "Started $session; attach: screen -r $session; log: $log_dir/$name.log"
    else
        echo "$session exited during startup; inspect $log_dir/$name.log" >&2
        status=1
    fi
done
exit "$status"
