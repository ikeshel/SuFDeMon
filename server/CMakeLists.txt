add_library(SuFDeMonServerLib
    src/TSuFDeMonServer.cxx
    src/SuFDeMonServerConfig.cxx
)

target_include_directories(SuFDeMonServerLib
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/inc
)

target_link_libraries(SuFDeMonServerLib
    PUBLIC
        SuFDeMonCommon
        ROOT::Core
        ROOT::RIO
        ROOT::Net
        ROOT::Hist
)

add_executable(SuFDeMonServer
    src/SuFDeMonServer.cxx
)

target_link_libraries(SuFDeMonServer
    PRIVATE
        SuFDeMonServerLib
)


# Dedicated entry points share the same transport and detector implementation.
foreach(detector MUSIC PLSCI SCIFI)
    add_executable(SuFDeMon${detector}Server src/SuFDeMonServer.cxx)
    target_compile_definitions(SuFDeMon${detector}Server PRIVATE
        SUFDEMON_DEFAULT_TYPE="${detector}" SUFDEMON_FIXED_TYPE)
    target_link_libraries(SuFDeMon${detector}Server PRIVATE SuFDeMonServerLib)
endforeach()

if(BUILD_TESTING)
    find_package(Python3 REQUIRED COMPONENTS Interpreter)
    add_executable(SuFDeMonServerProbe tests/ServerProbe.cxx)
    target_link_libraries(SuFDeMonServerProbe PRIVATE ROOT::Core ROOT::RIO ROOT::Net ROOT::Hist)
    add_test(NAME detector_servers
        COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tests/check_servers.py
            $<TARGET_FILE_DIR:SuFDeMonServer> ${PROJECT_SOURCE_DIR}
            $<TARGET_FILE:SuFDeMonServerProbe>)
    set_tests_properties(detector_servers PROPERTIES TIMEOUT 120)
endif()
