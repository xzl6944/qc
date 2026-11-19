qt_add_executable(App
    main.cpp
    mainwindow.cpp
    mainwindow.h

)
target_link_libraries(App PRIVATE
    simulator
    communication
    logic
    data
    ui
    Qt6::Core
    Qt6::Widgets
)