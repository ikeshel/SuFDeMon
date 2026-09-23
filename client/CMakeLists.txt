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
    MODULE SuFDeMonGui
)

add_executable(SuFDeMonGui
    src/SuFDeMonGui.cxx
    src/TSuFDeMonGui.cxx
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
