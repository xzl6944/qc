#include "mainwindow.h"
#include <QApplication>

// 根据你的 CMake 配置，资源文件的宏可能是下面这几个名字之一
// 通常以你的 qrc 文件名命名，或者是工程名

// 或者直接在 main 里面调用：
// Q_INIT_RESOURCE(ac); // 假设你的 qrc 文件叫 ac.qrc

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);

    // ===== 关键点：强制初始化资源 =====
    // 如果你的 qrc 文件叫ac.qrc，那么资源库名称就是 ac
    Q_INIT_RESOURCE(ac);
    // ==================================

    MainWindow w;
    w.show();

    return a.exec();
}