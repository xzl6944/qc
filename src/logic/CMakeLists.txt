qt_add_library(logic STATIC

    pidcontroller.h pidcontroller.cpp
    strategyengine.h strategyengine.cpp
    alarmengine.h alarmengine.cpp
    types.h
)
target_include_directories(logic PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(logic PUBLIC
    Qt6::Core
)