cmake_minimum_required(VERSION 3.16)

project(SuFDeMon VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

find_package(ROOT REQUIRED COMPONENTS Core RIO Net Hist Graf Gpad Rint Gui)

include(CTest)
if(BUILD_TESTING)
    find_package(Python3 REQUIRED COMPONENTS Interpreter)
endif()

add_subdirectory(common)
add_subdirectory(server)
add_subdirectory(client)
