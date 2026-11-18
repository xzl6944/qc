#ifndef ALARMENGINE_H
#define ALARMENGINE_H

#include <QObject>
#include <QString>
#include <QMap>

/**
 * @brief 报警记录结构体
 * 保存一条报警的完整信息
 */
struct AlarmRecord {
    int id;             // 报警唯一编号，自增
    QString message;    // 报警文字描述
    QString level;      // 报警等级："严重"、"中等"
    QString time;       // 报警触发时间（时分秒）
};

/**
 * @brief 报警引擎类
 * 负责各类报警逻辑判断：通信、温度、CO2、阀门卡涩
 * 检测到异常，组装AlarmRecord，发出newAlarmTriggered信号通知外部
 */
class AlarmEngine : public QObject {
    Q_OBJECT
public:
    explicit AlarmEngine(QObject *parent = nullptr);

    void checkSystemStatus(int zoneId, float currentTemp, bool isModbusConnected);
    void checkAirQuality(int zoneId, int co2Level); // 【修改】增加 zoneId
    void checkValveStuck(int zoneId, float valveOutput, float currentTemp);

signals:
    void newAlarmTriggered(const AlarmRecord &alarm);

private:
    void triggerAlarm(const QString &msg, const QString &level);
    int m_alarmCounter;
    QMap<int, float> m_lastTemps;
    QMap<int, int> m_stuckCounters;
};
#endif // ALARMENGINE_H