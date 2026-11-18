qt_add_library(ui STATIC

    chartmodule.h chartmodule.cpp
    zonemodel.h zonemodel.cpp
    alarmmodel.h alarmmodel.cpp

    ac.qrc

)

target_include_directories(ui PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(ui PUBLIC
    Qt6::Core
    Qt6::Widgets
    Qt6::Charts
    Qt6::QuickWidgets
    logic

)