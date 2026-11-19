#ifndef TYPES_H
#define TYPES_H

#include <QObject>
#include <QString>
#include <QMetaType>

// 冬夏模式切换
enum class ACMode {
    Cooling,    // 制冷
    Heating     // 制热
};

// 区域数据模型
struct ZoneData {
    int zoneId = 1;
    QString zoneName = "未命名";
    float currentTemp = 26.0f;
    float setTemp = 24.0f;     //根据策略的最终温度
    float userSetTemp = 24.0f;  //用户设置温度
    float valveOpening = 0.0f;
    float freshAirOpening = 10.0f; // 新风阀门开度 (0-100)
    float co2Level = 600.0f;
    bool isOccupied = true;
    bool isPowerOn = true;    // 设备开关状态
    ACMode mode = ACMode::Cooling;
};

// 注册元类型，使其可以在 Qt 信号槽中传递
Q_DECLARE_METATYPE(ZoneData)

#endif // TYPES_H