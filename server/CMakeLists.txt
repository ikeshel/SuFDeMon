add_library(SupFDetMonServerLib
    src/TSupFDetMonServer.cxx
)

target_include_directories(SupFDetMonServerLib
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/inc
)

target_link_libraries(SupFDetMonServerLib
    PUBLIC
        SupFDetMonCommon
        ROOT::Core
        ROOT::RIO
        ROOT::Net
        ROOT::Hist
)

add_executable(SupFDetMonServer
    src/SupFDetMonServer.cxx
)

target_link_libraries(SupFDetMonServer
    PRIVATE
        SupFDetMonServerLib
)
