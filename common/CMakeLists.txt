add_library(SupFDetMonCommon
    src/TSupFDetMonMessage.cxx
)

target_include_directories(SupFDetMonCommon
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/inc
)

target_link_libraries(SupFDetMonCommon
    PUBLIC
        ROOT::Core
        ROOT::RIO
        ROOT::Net
)
