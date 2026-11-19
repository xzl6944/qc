#ifndef MAINWINDOW_H
#define MAINWINDOW_H
#include <QMainWindow>
#include <QListWidget>
#include <QStackedWidget>
#include <QQuickWidget>
#include <QTimer>
#include <QLabel>
#include <QVariantList>
#include <QVariantMap>
#include <QMap>
#include <QModbusDataUnit> // 必须包含，因为新函数用到了这个类型
#include "types.h"

// 前置声明，减少头文件依赖，加快编译速度
class ModbusMaster; class ZoneModel; class AlarmModel; class ChartModule;
class EnergyCalc; class PIDController; class DataLogger;
class StrategyEngine; class AlarmEngine; class RoomModel;

class MainWindow : public QMainWindow
{
    Q_OBJECT
public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

    // 供 QML 调用的历史数据查询接口（分页）
    Q_INVOKABLE QVariantMap queryHistory(const QString &date, int page, int pageSize);
    Q_INVOKABLE QStringList getRecentDates();
    Q_INVOKABLE bool exportHistoryToCSV(const QString &date);
private slots:
    // 【核心槽函数】处理 Modbus 读回调，1秒触发一次，包含所有业务逻辑
    void onModbusDataReceived(const QModbusDataUnit &unit);

private:
    QWidget* createQmlTab(const QString& qmlPath, const QString& contextName, QObject* contextObject);
    void setupUi();

    // --- UI 组件 ---
    QListWidget *m_navList;       // 左侧导航栏
    QStackedWidget *m_stack;      // 右侧页面堆叠容器
    QLabel *m_timeLabel;          // 顶栏时间
    QLabel *m_statusLabel;        // 顶栏通信状态指示灯
    QLabel *m_alarmLightLabel;    // 顶栏报警状态指示灯

    // --- 核心业务模块 ---
    ModbusMaster *m_modbus;       // 通信主站
    QTimer *m_pollTimer;          // 1秒心跳定时器
    ZoneModel *m_zoneModel;       // 区域数据模型（给UI用）
    AlarmModel *m_alarmModel;     // 报警数据模型（给UI用）
    ChartModule *m_chartModel;    // 曲线数据模型（给UI用）
    EnergyCalc *m_energyCalc;     // 能耗统计计算
    DataLogger *m_logger;         // SQLite 数据库持久化

    // --- 引擎与仿真 ---
    StrategyEngine *m_strategy;   // 节能策略引擎
    AlarmEngine *m_alarmEngine;   // 报警检测引擎
    RoomModel *m_roomSimulator;   // 物理房间仿真模型
    QMap<int, PIDController*> m_zonePids; // 4个区域独立的 PID 控制器

    QList<ZoneData> m_currentZones; // 当前所有区域的最新状态缓存
    ACMode m_currentMode;           // 全局当前模式（预留）
};
#endif // MAINWINDOW_H