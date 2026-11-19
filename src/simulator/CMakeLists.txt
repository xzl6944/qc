qt_add_library(simulator STATIC

    roommodel.h roommodel.cpp
    
)
target_include_directories(simulator PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(simulator PUBLIC
    Qt6::Core
    logic
)