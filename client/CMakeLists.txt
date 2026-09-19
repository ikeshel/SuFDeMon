add_library(SupFDetMonClientLib
    src/TSupFDetMonClient.cxx
)

target_include_directories(SupFDetMonClientLib
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/inc
)

target_link_libraries(SupFDetMonClientLib
    PUBLIC
        SupFDetMonCommon
        ROOT::Core
        ROOT::RIO
        ROOT::Net
        ROOT::Hist
        ROOT::Graf
        ROOT::Gpad
)

add_executable(SupFDetMonClient
    src/SupFDetMonClient.cxx
)

set_target_properties(SupFDetMonClient PROPERTIES ENABLE_EXPORTS ON)

target_link_libraries(SupFDetMonClient
    PRIVATE
        SupFDetMonClientLib
        ROOT::Rint
)

add_executable(SupFDetMonGui
    src/SupFDetMonGui.cxx
    src/TSupFDetMonGui.cxx
)

target_include_directories(SupFDetMonGui
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/inc
)

target_link_libraries(SupFDetMonGui
    PRIVATE
        SupFDetMonClientLib
        ROOT::Gui
        ROOT::Gpad
        ROOT::Hist
)
