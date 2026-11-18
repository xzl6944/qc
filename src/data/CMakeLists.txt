qt_add_library(data STATIC

    datalogger.h datalogger.cpp
    energycalc.h energycalc.cpp
)
target_include_directories(data PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(data PUBLIC
    Qt6::Core
    Qt6::Sql
    logic
)