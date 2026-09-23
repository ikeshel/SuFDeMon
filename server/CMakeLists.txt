add_library(SuFDeMonServerLib
    src/TSuFDeMonServer.cxx
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
