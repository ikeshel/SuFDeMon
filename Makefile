# SuFDeMon convenience Makefile
# The actual build system is CMake.

BUILD_DIR  ?= build
BUILD_TYPE ?= RelWithDebInfo

HOST ?= localhost
PORT ?= 10001
TYPE ?= MUSIC
INSTANCE ?= $(TYPE)1
CONFIG ?= config/servers/$(INSTANCE).conf

CMAKE ?= cmake

.PHONY: all configure build debug release clean rebuild \
        server client gui run-server run-client run-gui \
        pull help servers music-server plsci-server scifi-server run-instance

all: build

configure:
	$(CMAKE) -S . -B $(BUILD_DIR) \
		-DCMAKE_BUILD_TYPE=$(BUILD_TYPE)

build: configure
	$(CMAKE) --build $(BUILD_DIR) --parallel

debug:
	$(MAKE) BUILD_TYPE=Debug build

release:
	$(MAKE) BUILD_TYPE=Release build

server: configure
	$(CMAKE) --build $(BUILD_DIR) --target SuFDeMonServer --parallel

client: configure
	$(CMAKE) --build $(BUILD_DIR) --target SuFDeMonClient --parallel

gui: configure
	$(CMAKE) --build $(BUILD_DIR) --target SuFDeMonGui --parallel

run-server: server
	./$(BUILD_DIR)/server/SuFDeMonServer $(PORT)

run-client: client
	./$(BUILD_DIR)/client/SuFDeMonClient $(HOST) $(PORT)

run-gui: gui
	./$(BUILD_DIR)/client/SuFDeMonGui $(HOST) $(PORT)

clean:
	$(CMAKE) -E remove_directory $(BUILD_DIR)

rebuild: clean build

pull:
	git pull --ff-only

help:
	@echo "SuFDeMon build targets:"
	@echo ""
	@echo "  make              Build everything"
	@echo "  make build        Build everything"
	@echo "  make debug        Build with Debug configuration"
	@echo "  make release      Build with Release configuration"
	@echo "  make server       Build server only"
	@echo "  make client       Build command-line client only"
	@echo "  make gui          Build GUI only"
	@echo "  make run-server   Build and run server"
	@echo "  make run-client   Build and run client"
	@echo "  make run-gui      Build and run GUI"
	@echo "  make clean        Remove build directory"
	@echo "  make rebuild      Clean and rebuild"
	@echo "  make pull         Git pull using fast-forward only"
	@echo ""
	@echo "  make servers      Build MUSIC, PLSCI and SCIFI servers"
	@echo "  make run-instance INSTANCE=MUSIC2  Run an instance config"
	@echo "Variables:"
	@echo "  BUILD_DIR=build"
	@echo "  BUILD_TYPE=RelWithDebInfo"
	@echo "  HOST=localhost"
	@echo "  PORT=10001"


servers: configure
	$(CMAKE) --build $(BUILD_DIR) --target SuFDeMonMUSICServer SuFDeMonPLSCIServer SuFDeMonSCIFIServer --parallel

music-server: configure
	$(CMAKE) --build $(BUILD_DIR) --target SuFDeMonMUSICServer --parallel

plsci-server: configure
	$(CMAKE) --build $(BUILD_DIR) --target SuFDeMonPLSCIServer --parallel

scifi-server: configure
	$(CMAKE) --build $(BUILD_DIR) --target SuFDeMonSCIFIServer --parallel

run-instance: server
	./$(BUILD_DIR)/server/SuFDeMonServer --config "$(CONFIG)"
