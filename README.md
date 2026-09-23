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
  - 32 ADC channels per field cage
  - 96 `TH1D` histograms
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
| MUSIC | `SuFDeMonMUSICServer` | MUSIC1, MUSIC2 | Existing 3 × 32 simulated ADC histograms |
| PLSCI | `SuFDeMonPLSCIServer` | PLSCI1, PLSCI2, PLSCI3 | Runnable skeleton; detector definitions pending |
| SCIFI | `SuFDeMonSCIFIServer` | SCIFI1–SCIFI14 | Runnable skeleton; detector definitions pending |

Build with `make servers` or the normal CMake build. Each process owns its
histograms independently. Instance names do not change MUSIC histogram names,
so existing MUSIC clients continue to work.

The files under `config/servers/` define `type`, `instance`, `hostname` and `port`.
Replace the `.example.invalid` hostnames with real hostnames or IP addresses.
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

PLSCI and SCIFI support `INFO`, `PING`, `LIST`, `GET`, `CLEAR`, `CLEAR ALL`,
`QUIT` and `SHUTDOWN`, but return an empty histogram list until their definitions
are implemented. No detector channels or physics data have been invented.
The current GUI remains MUSIC-specific; its host/port fields select a MUSIC
instance. A detector-specific GUI for PLSCI/SCIFI is future work.

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
TH1D_MUSIC_ADC_FC1_ADC0
TH1D_MUSIC_ADC_FC1_ADC1
...
TH1D_MUSIC_ADC_FC3_ADC31
```

The prototype server fills these histograms with simulated data.

## ROOT GUI client

With the server running:

```bash
./build/client/SuFDeMonGui localhost 10001
```

The GUI attempts to connect automatically using the host and port supplied on the command line. If the connection is unavailable, the GUI remains open and the connection can be retried manually.

The control window provides:

- server host and port
- Connect / Disconnect
- MUSIC field-cage selection
- ADC-channel selection
- Draw
- Clear
- Clear All
- Auto update
- configurable update interval

**Auto update is enabled by default with a 1.0 second interval.** After a histogram has been drawn, a ROOT `TTimer` periodically requests a fresh snapshot from the server and redraws the existing canvas.

The histogram canvas is intentionally a normal, separate ROOT `TCanvas`, rather than being embedded in the control window.

The GUI application also runs `TRint`, so the normal interactive ROOT prompt remains available while the GUI is running.

## Interactive ROOT client

The command-line client can be started with:

```bash
./build/client/SuFDeMonClient localhost 10001
```

It provides a normal ROOT prompt with SuFDeMon helper functions. For example:

```cpp
root [0] SuFDeMonPing()
root [1] SuFDeMonList()
root [2] TH1D* h = SuFDeMonGet("TH1D_MUSIC_ADC_FC1_ADC0")
root [3] h->Draw()
root [4] h->GetEntries()
root [5] h->GetMean()
root [6] SuFDeMonClear("TH1D_MUSIC_ADC_FC1_ADC0")
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

## Current MUSIC naming convention

MUSIC ADC histogram names follow:

```text
TH1D_MUSIC_ADC_FC<field-cage>_ADC<channel>
```

where:

- `field-cage` is 1-3
- `channel` is 0-31

Example:

```text
TH1D_MUSIC_ADC_FC2_ADC15
```

Shared detector constants and naming helpers are kept under `common/` so that the server and clients use exactly the same definitions.

## Development direction

SuFDeMon is being developed toward a general Super-FRS detector monitoring application. Planned work includes integration of real detector data sources, support for additional detector systems, richer histogram selection and organization, multi-histogram displays, and improved online monitoring controls.

The current MUSIC implementation is intentionally small and provides the foundation for those extensions.

## License

SuFDeMon is released under the **GNU General Public License v3.0 (GPL-3.0)**. See `LICENSE` for details.

