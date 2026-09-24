add_library(SuFDeMonClientLib
    src/TSuFDeMonClient.cxx
)

target_include_directories(SuFDeMonClientLib
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/inc
)

target_link_libraries(SuFDeMonClientLib
    PUBLIC
        SuFDeMonCommon
        ROOT::Core
        ROOT::RIO
        ROOT::Net
        ROOT::Hist
        ROOT::Graf
        ROOT::Gpad
)

add_executable(SuFDeMonClient
    src/SuFDeMonClient.cxx
)

set_target_properties(SuFDeMonClient PROPERTIES ENABLE_EXPORTS ON)

target_link_libraries(SuFDeMonClient
    PRIVATE
        SuFDeMonClientLib
        ROOT::Rint
)

ROOT_GENERATE_DICTIONARY(G__SuFDeMonGui
    ${CMAKE_CURRENT_SOURCE_DIR}/inc/TSuFDeMonGui.h
    LINKDEF ${CMAKE_CURRENT_SOURCE_DIR}/inc/SuFDeMonGuiLinkDef.h
    MODULE SuFDeMonGui
)

add_executable(SuFDeMonGui
    src/SuFDeMonGui.cxx
    src/TSuFDeMonGuiLayout.cxx
    src/TSuFDeMonGuiServerConnections.cxx
    src/TSuFDeMonGuiActions.cxx
    ${CMAKE_CURRENT_BINARY_DIR}/G__SuFDeMonGui.cxx
)

set_target_properties(SuFDeMonGui PROPERTIES ENABLE_EXPORTS ON)

target_include_directories(SuFDeMonGui
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/inc
        ${CMAKE_CURRENT_BINARY_DIR}
)

target_link_libraries(SuFDeMonGui
    PRIVATE
        SuFDeMonClientLib
        ROOT::Gui
        ROOT::Gpad
        ROOT::Hist
        ROOT::Rint
)

if(BUILD_TESTING)
    add_executable(SuFDeMonDisconnectProbe tests/DisconnectProbe.cxx)
    target_link_libraries(SuFDeMonDisconnectProbe PRIVATE SuFDeMonClientLib)
    add_test(NAME client_disconnect
        COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tests/check_disconnect.py
            $<TARGET_FILE:SuFDeMonMUSICServer> $<TARGET_FILE:SuFDeMonDisconnectProbe>)
    set_tests_properties(client_disconnect PROPERTIES TIMEOUT 40)
    add_test(NAME gui_connections
        COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tests/check_gui_connections.py
            ${PROJECT_SOURCE_DIR} ${PROJECT_BINARY_DIR})
    set_tests_properties(gui_connections PROPERTIES TIMEOUT 90 SKIP_RETURN_CODE 77 LABELS "gui")
endif()
