# SuFDeMon

**SuFDeMon** is a C++/CERN ROOT based client-server application for monitoring detectors of the **Super-FRS**.

The project provides a ROOT network server that owns detector histograms and remote clients that can retrieve, display, refresh, and clear those histograms. Both an interactive ROOT command-line client and a ROOT GUI client are provided.

> **Project status:** early development / prototype. The current implementation uses simulated MUSIC ADC data while the monitoring architecture is being developed.

## Features

- C++17 and CERN ROOT
- ROOT socket-based client-server communication
- Server-side authoritative histograms
- Serialization and transfer of ROOT histograms over the network
- MUSIC prototype with:
  - 3 field cages (FC1-FC3)
  - 32 ADC and 32 TDC channels per field cage
  - 192 `TH1D` histograms
- Interactive ROOT (`TRint`) client
- ROOT GUI control application
- Separate standard ROOT `TCanvas` for histogram display
- Automatic histogram refresh using `TTimer`
- Configurable refresh interval (default: 1 s)
- Remote clearing of individual histograms or all histograms
- Automatic connection attempt when the GUI starts

## Architecture

```text
                         ROOT socket connection
+------------------+    <---------------------->    +------------------+
|                  |                               |                  |
| SuFDeMonServer |                               | SuFDeMonClient |
|                  |                               |                  |
| owns/fills TH1D  |                               | ROOT / TRint CLI |
+------------------+                               +------------------+
        ^
        |                                           
        | ROOT socket connection                    
        v                                           
+------------------+       +------------------+
| SuFDeMon GUI   | ----> | ROOT TCanvas     |
|                  |       |                  |
| connection       |       | histogram view   |
| selection        |       +------------------+
| auto refresh     |
| clear controls   |
+------------------+
```

The server owns the authoritative histogram objects. Clients receive serialized snapshots. Operations such as `CLEAR` are sent back to the server and modify the server-side histogram.

## Repository layout

```text
SuFDeMon/
├── common/     Shared protocol, messages, detector names and constants
├── server/     ROOT histogram server
├── client/     Interactive client and ROOT GUI client
├── CMakeLists.txt
├── LICENSE
└── README.md
```

## Requirements

- CMake 3.16 or newer
- C++17 compatible compiler
- CERN ROOT with the following components:
  - Core
  - RIO
  - Net
  - Hist
  - Graf
  - Gpad
  - Rint
  - Gui

ROOT must be installed and discoverable by CMake. If necessary, source your ROOT environment before configuring the project.

For example:

```bash
source /path/to/root/bin/thisroot.sh
```

## Build

Clone the repository:

```bash
git clone git@github.com:ikeshel/SuFDeMon.git
cd SuFDeMon
```

Configure and build:

```bash
cmake -S . -B build
cmake --build build -j$(nproc)
```

The main executables are then located at:

```text
build/server/SuFDeMonServer
build/client/SuFDeMonClient
build/client/SuFDeMonGui
```

## Detector server types and instances

Three dedicated executables share the server transport and protocol:

| Type | Executable | Example instances | Histogram status |
| --- | --- | --- | --- |
| MUSIC | `SuFDeMonMUSICServer` | MUSIC1, MUSIC2 | 3 × 32 ADC + 3 × 32 TDC histograms |
| PLSCI | `SuFDeMonPLSCIServer` | PLSCI1, PLSCI2, PLSCI3 | 32 ADC + 32 TDC histograms (provisional layout) |
| SCIFI | `SuFDeMonSCIFIServer` | SCIFI1–SCIFI14 | 32 ADC + 32 TDC histograms (provisional layout) |

Build with `make servers` or the normal CMake build. Each process owns its
histograms independently. Histogram names include the instance name.

The files under `config/servers/` define `type`, `instance`, `hostname` and `port`.
All example hostnames are currently `localhost` for local testing. Replace them
with the readout PCs' hostnames or IP addresses when deploying remotely.
`hostname` advertises the endpoint in startup output and the `INFO` response;
it does **not** configure DNS, select a bind address, or start a remote process.
Servers listen on all local interfaces. Run each command on the corresponding
server machine:

```bash
./build/server/SuFDeMonMUSICServer --config config/servers/MUSIC1.conf
./build/server/SuFDeMonMUSICServer --config config/servers/MUSIC2.conf
./build/server/SuFDeMonPLSCIServer --config config/servers/PLSCI1.conf
./build/server/SuFDeMonPLSCIServer --config config/servers/PLSCI2.conf
./build/server/SuFDeMonPLSCIServer --config config/servers/PLSCI3.conf
./build/server/SuFDeMonSCIFIServer --config config/servers/SCIFI1.conf
# Likewise SCIFI2.conf through SCIFI14.conf, each on its readout PC.
```

Example ports are MUSIC `10001–10002`, PLSCI `10101–10103`, and SCIFI
`10201–10214`; each can be changed independently. The same port also works on
different machines. For multiple processes on
one machine, assign distinct ports. Command-line options override config values:

```bash
./build/server/SuFDeMonMUSICServer --config config/servers/MUSIC2.conf --hostname localhost --port 10002
make run-instance INSTANCE=PLSCI3
```

`SuFDeMonServer` is also a generic entry point, retaining the old `[port]` syntax
(default MUSIC1), and accepting `--type`, `--instance`, `--hostname`, `--port`
and `--config`. Dedicated executables reject configs for another detector type.
Use `--help` for syntax. Copy a config and change its instance and hostname to
add more instances; the counts are not hardcoded.

All detector types support `INFO`, `PING`, `LIST`, `GET`, `CLEAR`, `CLEAR ALL`,
`QUIT` and `SHUTDOWN`. The GUI supports ADC/TDC selection, drawing, clearing,
and automatic updates for all types; field cages apply only to MUSIC.

All histograms currently contain simulated data. ADC and TDC use 4096 bins
from 0 to 4096 in raw counts; TDC values are not calibrated time units.
PLSCI and SCIFI provisionally use channels 0–31 for each quantity, pending
hardware-specific channel maps and timing ranges.

### Running servers in Screen

Install GNU Screen and `flock` (util-linux), build the servers, and source your
ROOT environment as usual. Run these scripts on each detector readout PC:

```bash
# Start selected configurations on this PC.
./scripts/start_servers.sh MUSIC1
./scripts/start_servers.sh PLSCI2 SCIFI14

# For local testing, start all 19 configurations.
./scripts/start_servers.sh

screen -ls
screen -r SuFDeMon-MUSIC1
# Detach with Ctrl-A, then D; the server keeps running.

# Stop all SuFDeMon-* sessions owned by this user on this PC.
./scripts/stop_servers.sh

# Or stop only the named instances.
./scripts/stop_servers.sh MUSIC1
./scripts/stop_servers.sh MUSIC2 PLSCI3 SCIFI14
```

Session names are `SuFDeMon-<config filename without .conf>`. Existing sessions
are skipped, and logs are written to `logs/servers/<name>.log`. The scripts work
from any working directory. `BUILD_DIR`, `CONFIG_DIR`, and `LOG_DIR` can override
the start script's default directories (relative overrides use your current
working directory).

The scripts launch local processes; config hostnames do not trigger SSH or
remote deployment. The stop script closes Screen sessions, terminating the
processes inside them. It does not send the ROOT `SHUTDOWN` command, stop servers
launched outside these sessions, or affect unrelated Screen sessions. Reserve
the `SuFDeMon-` session prefix for these servers.

### PyQt6 server control window

```bash
python3 -m pip install -r control/requirements.txt
python3 control/server_control.py
```

The top row starts or stops all local servers. The second row has MUSIC (2),
PLSCI (3), and SCIFI (14) columns with one toggle button per instance. Buttons
show the local Screen-session state and start or stop that server when clicked.
See [control/README.md](control/README.md) for requirements, behavior, and tests.

### Server checks

With testing enabled (the default), run `ctest --test-dir build --output-on-failure`.
The integration test launches all 19 example instances locally on separate
ports, checks their identity and histogram behavior, and shuts them down.

## Running the server

The default TCP port is **10001**.

Start the server with:

```bash
./build/server/SuFDeMonServer
```

or specify a port explicitly:

```bash
./build/server/SuFDeMonServer 10001
```

For the current MUSIC prototype the server creates 96 histograms using names such as:

```text
hMUSIC1_FC1_ADC0
hMUSIC1_FC1_ADC1
...
hMUSIC1_FC3_ADC31
```

The prototype server fills these histograms with simulated data.

## ROOT GUI client

With the server running:

```bash
./build/client/SuFDeMonGui localhost 10001
```

At startup the GUI attempts to connect to every configured server. Unavailable servers remain disconnected and can be retried with their individual or group controls. The command-line host and port choose the preferred histogram server when connected; otherwise the first available server is selected. Connected server labels use dark green for readability. An independent one-second connection check detects a closed server even when histogram auto-update is disabled, changes the status to Disconnected, and enables reconnection. Transport send/receive failures also invalidate the connection. This detects socket closure/reset; it is not a heartbeat for silent network outages.

The control window provides:

- independent connections to all configured servers simultaneously
- MUSIC, PLSCI, and SCIFI columns with **Connect all** / **Disconnect all**
- individual server buttons to toggle each connection
- connection summary, global **Connect all** / **Disconnect all**, and Refresh Status
  at the top of the Server Connections tab
- separate **MUSIC**, **PLSCI**, and **SCIFI** drawing tabs, each with its own
  detector-filtered server selector, ADC/TDC selection, channel, and refresh settings
- selections retained when switching detector tabs; connections stay open
- MUSIC field-cage selection
- ADC/TDC quantity and channel selection
- Draw
- Clear
- Clear All
- Auto update
- configurable update interval

**Auto update is enabled by default with a 1.0 second interval.** After a histogram has been drawn, a ROOT `TTimer` periodically requests a fresh snapshot from the server and redraws the existing canvas.

General retains status and process controls; Server Connections retains the shared
connection groups. MUSIC alone has a field-cage selector. Switching detector tabs
pauses automatic drawing until Draw is clicked for that selection.

The histogram canvas is intentionally a normal, separate ROOT `TCanvas`, rather than being embedded in the control window.

The GUI application also runs `TRint`, so the normal interactive ROOT prompt remains available while the GUI is running. Use `ListOfHistograms()` there to list histogram names from all connected servers, in detector and instance order. The histogram-server selection does not restrict this list.

Group disconnects close client sockets only; server processes keep running.
Closing the client releases all its connections. **Close server** shuts down the
selected histogram server; **Close All** shuts down all connected servers and
closes the client.

The `gui_connections` CTest integration test uses temporary server configurations
and isolated ports to exercise all 19 connections. It needs an X display and is
skipped when `DISPLAY` is unset; use `xvfb-run ctest --test-dir build -R gui_connections`
for a virtual display.

## Interactive ROOT client

The command-line client can be started with:

```bash
./build/client/SuFDeMonClient localhost 10001
```

It provides a normal ROOT prompt with SuFDeMon helper functions. For example:

```cpp
root [0] SuFDeMonPing()
root [1] ListOfHistograms()
root [2] TH1D* h = SuFDeMonGet("hMUSIC1_FC1_ADC0")
root [3] h->Draw()
root [4] h->GetEntries()
root [5] h->GetMean()
root [6] SuFDeMonClear("hMUSIC1_FC1_ADC0")
root [7] SuFDeMonClearAll()
```

The returned histogram is a ROOT object, so standard ROOT operations can be used on it.

## Network protocol

The current lightweight command protocol supports:

```text
INFO
PING
LIST
GET <histogram-name>
CLEAR <histogram-name>
CLEAR ALL
QUIT
SHUTDOWN
```

Histogram objects are transferred using ROOT serialization.

## Histogram naming convention

MUSIC ADC/TDC histogram names follow:

```text
h<instance>_FC<field-cage>_<ADC|TDC><channel>
```

where:

- `instance` is the server name, such as `MUSIC1` or `MUSIC2`
- `field-cage` is 1-3
- `channel` is 0-31

Example:

```text
hMUSIC1_FC2_ADC15
hMUSIC2_FC2_TDC26
hSCIFI1_ADC0
hSCIFI1_TDC0
hPLSCI1_ADC0
hPLSCI1_TDC0
```

PLSCI and SCIFI names omit the field-cage component.

Shared detector constants and naming helpers are kept under `common/` so that the server and clients use exactly the same definitions.

## Development direction

SuFDeMon is being developed toward a general Super-FRS detector monitoring application. Planned work includes integration of real detector data sources, support for additional detector systems, richer histogram selection and organization, multi-histogram displays, and improved online monitoring controls.

The current MUSIC implementation is intentionally small and provides the foundation for those extensions.

## License

SuFDeMon is released under the **GNU General Public License v3.0 (GPL-3.0)**. See `LICENSE` for details.

After rebuilding, restart server processes to publish the new histogram names. The GUI also recognizes names from older running servers.
