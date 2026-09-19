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

target_link_libraries(SupFDetMonClient
    PRIVATE
        SupFDetMonClientLib
)
