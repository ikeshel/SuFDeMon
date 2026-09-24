# SuFDeMon control windows

`server_control.py` is a PyQt6 window for local detector server processes.
It uses the existing `scripts/start_servers.sh` and `scripts/stop_servers.sh`.

## Run

Requirements: Python 3.10+, PyQt6, Bash, GNU Screen, `flock`, and built server
executables. Source the ROOT environment before launching, just as for the scripts.
From the repository root:

```bash
python3 -m pip install -r control/requirements.txt
make servers
python3 control/server_control.py
```

The script resolves the repository from its own location, so it also works when
launched from another working directory. Existing `BUILD_DIR`, `CONFIG_DIR`,
`LOG_DIR`, and `SCREENDIR` environment overrides are inherited by the scripts.

## Layout and actions

- Top row: **Start All**, **Stop All**, **Refresh Status**, and **Script Output**.
- Second row: **MUSIC**, **PLSCI**, and **SCIFI** columns, containing respectively
  2, 6, and 14 vertically stacked buttons from the configuration files.
- Each column begins with **Start all** and **Stop all**, affecting only the
  configured servers in that detector group. Empty groups have disabled controls.
- A green **Running---Stop** button stops that instance when clicked.
- A gray **Stopped---Start** button starts that instance when clicked.
- Unavailable status disables an individual button until Screen can be queried.
- Hover over a button to see its configured hostname, port, and session name.

Status updates every two seconds from `screen -ls`. This indicates the presence
of the local managed Screen session, not detector/network health. It detects
sessions started or stopped outside the window. Stale Screen sessions appear
unavailable and must be inspected with `screen -ls`. Config changes are loaded
when the window opens; restart the window after adding or changing configs.

Commands run asynchronously and conflicting actions are disabled until the
current script finishes. Script output is retained in a separate window, which
opens automatically on failure. A window close is deferred while a script is
running. Closing an idle window leaves the detached servers running; use
**Stop All** to stop them.

All actions affect the current user's local Screen sessions. Hostnames in the
configuration files do not cause remote execution. Start All uses every config
in `CONFIG_DIR`; Stop All closes all local `SuFDeMon-*` sessions for this user,
just like the command-line scripts.

## Tests

```bash
QT_QPA_PLATFORM=offscreen python3 -m unittest discover -s control/tests -v
```

Tests exercise the actual GUI and shell scripts with an isolated Screen
substitute, without starting or stopping real detector processes.
