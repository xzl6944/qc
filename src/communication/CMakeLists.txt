qt_add_library(communication STATIC

    modbusmaster.h modbusmaster.cpp


)
target_include_directories(communication PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(communication PUBLIC
    Qt6::Core
    Qt6::Network
    Qt6::SerialBus

)