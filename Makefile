# SupFDetMon convenience Makefile
# The actual build system is CMake.

BUILD_DIR  ?= build
BUILD_TYPE ?= RelWithDebInfo

HOST ?= localhost
PORT ?= 10001

CMAKE ?= cmake

.PHONY: all configure build debug release clean rebuild \
        server client gui run-server run-client run-gui \
        pull help

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
	$(CMAKE) --build $(BUILD_DIR) --target SupFDetMonServer --parallel

client: configure
	$(CMAKE) --build $(BUILD_DIR) --target SupFDetMonClient --parallel

gui: configure
	$(CMAKE) --build $(BUILD_DIR) --target SupFDetMonGui --parallel

run-server: server
	./$(BUILD_DIR)/server/SupFDetMonServer $(PORT)

run-client: client
	./$(BUILD_DIR)/client/SupFDetMonClient $(HOST) $(PORT)

run-gui: gui
	./$(BUILD_DIR)/client/SupFDetMonGui $(HOST) $(PORT)

clean:
	$(CMAKE) -E remove_directory $(BUILD_DIR)

rebuild: clean build

pull:
	git pull --ff-only

help:
	@echo "SupFDetMon build targets:"
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
	@echo "Variables:"
	@echo "  BUILD_DIR=build"
	@echo "  BUILD_TYPE=RelWithDebInfo"
	@echo "  HOST=localhost"
	@echo "  PORT=10001"
