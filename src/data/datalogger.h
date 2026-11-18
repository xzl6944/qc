#ifndef DATALOGGER_H       // 头文件保护宏，防止头文件重复包含
#define DATALOGGER_H

#include <QObject>         // Qt基类，支持信号槽、元对象系统
#include <QSqlDatabase>    // Qt SQL数据库操作类
#include <QList>           // Qt容器，泛型链表容器
#include <QVariantMap>     // Qt键值容器，可给QML传递数据
#include "types.h"         // 自定义类型头文件，包含ZoneData等结构体

/**
 * @brief DataLogger 数据日志持久化类，继承QObject，供Qt/QML调用
 * 负责sqlite数据库初始化、日志存储、历史分页查询、设备配置读写、告警记录
 */
class DataLogger : public QObject {
    Q_OBJECT               // Qt元对象宏，启用信号槽、Q_INVOKABLE等特性
public:
    // 构造函数，父对象可选，Qt对象树内存管理
    explicit DataLogger(QObject *parent = nullptr);

    // 初始化数据库：打开数据库文件、建表、初始化默认配置，成功返回true
    bool initDatabase();

    // 记录所有分区实时运行数据
    void logRealtimeZones(const QList<ZoneData> &zones);

    // 记录系统总能耗；增加总功率入参 totalPower
    void logTotalEnergy(float totalKwh, float savedKwh, float co2Red, float totalPower);

    // 写入告警日志：告警等级、告警文本消息
    void logAlarm(int level, const QString &message);

    // 保存分区设备配置到数据库
    void saveDeviceConfig(const QList<ZoneData> &zones);

    // 读取设备配置
    QList<ZoneData> loadDeviceConfig();

    /**
     * @brief 分页查询指定日期总能耗历史记录，Q_INVOKABLE允许QML直接调用该C++方法
     * @param dateStr 日期字符串 "yyyy‑MM‑dd"
     * @param page 当前页码，从1开始
     * @param pageSize 每页条数
     * @return QVariantMap 包含dataList数据、totalPages总页数、totalCount总条数
     */
    Q_INVOKABLE QVariantMap getTotalEnergyHistory(const QString &dateStr, int page, int pageSize);

private:
    QSqlDatabase m_db;     // SQLite数据库连接实例
};

#endif // DATALOGGER_H
