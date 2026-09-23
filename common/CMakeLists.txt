add_library(SuFDeMonCommon
    src/TSuFDeMonMessage.cxx
)

target_include_directories(SuFDeMonCommon
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/inc
)

target_link_libraries(SuFDeMonCommon
    PUBLIC
        ROOT::Core
        ROOT::RIO
        ROOT::Net
)
