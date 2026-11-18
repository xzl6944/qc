#include "alarmengine.h"
#include <QDateTime>
#include <QDebug>
#include <cmath>

/**
 * @brief 构造函数
 * @param parent 父对象
 * 初始化报警计数器从0开始
 */
AlarmEngine::AlarmEngine(QObject *parent) : QObject(parent), m_alarmCounter(0) {}

/**
 * @brief 系统状态检测
 * 1. Modbus断开 → 严重报警
 * 2. 温度超出[-50,90]范围 → 传感器异常严重报警
 * @param zoneId 区域ID
 * @param currentTemp 当前温度
 * @param isModbusConnected modbus连接状态
 */
void AlarmEngine::checkSystemStatus(int zoneId, float currentTemp, bool isModbusConnected) {
    if (!isModbusConnected) {
        triggerAlarm("Modbus 通信中断，系统正在重试", "严重");
        return;
    }
    if (currentTemp < -50.0f || currentTemp > 90.0f) {
        triggerAlarm(QString("区域 %1 温度传感器数据异常").arg(zoneId), "严重");
    }
}
/**
 * @brief CO2空气质量检测
 * @param zoneId 区域编号
 * @param co2Level CO2浓度值
 * 大于1000触发中等报警，提示开启新风
 */
void AlarmEngine::checkAirQuality(int zoneId, int co2Level) {
    if (co2Level > 1000) {
        if (co2Level > 1200) {
            triggerAlarm(QString("区域 %1 室内 CO2 浓度严重超标 (%2 ppm)").arg(zoneId).arg(co2Level), "严重");
        } else {
            triggerAlarm(QString("区域 %1 室内 CO2 浓度过高 (%2 ppm)").arg(zoneId).arg(co2Level), "中等");
        }
    }
}
/**
 * @brief 阀门卡涩检测逻辑
 * 逻辑：阀门开度明显变化了，但温度几乎不变，多次连续出现判定阀门卡涩
 * @param zoneId 区域
 * @param valveOutput 当前阀门开度(%)
 * @param currentTemp 当前温度
 */
void AlarmEngine::checkValveStuck(int zoneId, float valveOutput, float currentTemp) {
    // static局部静态变量：函数调用之间数据保留；key区域ID，value上一次阀门开度
    static QMap<int, float> lastValves;
    // 判断：已经保存过该区域上一次温度 AND 保存过上一次阀门开度
    if (m_lastTemps.contains(zoneId) && lastValves.contains(zoneId)) {
        // 取出上一轮温度、上一轮阀门开度
        float lastTemp = m_lastTemps[zoneId];
        float lastValve = lastValves[zoneId];
        // 判断阀门是否动作：本次和上次开度差大于15% → 认为阀门有动作
        bool valveMoving = std::abs(valveOutput - lastValve) > 15.0f;
        // 条件：阀门已经动了，但是温度几乎没变化（差值小于0.02）
        if (valveMoving && std::abs(currentTemp - lastTemp) < 0.02f) {
            m_stuckCounters[zoneId]++;
            // 连续累计达到5次，判定阀门卡涩，触发报警
            if (m_stuckCounters[zoneId] >= 5) {
                // static_cast<int> 把浮点开度转成整数展示
                triggerAlarm(QString("区域 %1 阀门卡涩：开度 %2% 但温度无响应").arg(zoneId).arg(static_cast<int>(valveOutput)), "中等");
                // 报警发出后，计数器清零，等待下一轮累积
                m_stuckCounters[zoneId] = 0;
            }
        } else {
            // 条件不满足（温度跟随变化 / 阀门没有动），异常计数器清零
            m_stuckCounters[zoneId] = 0;
        }
    }
    m_lastTemps[zoneId] = currentTemp;
    lastValves[zoneId] = valveOutput;
}

/**
 * @brief 内部通用报警生成函数
 * 组装AlarmRecord结构体，打上id、消息、等级、时间，发出信号
 * @param msg 报警文本内容
 * @param level 报警等级 "严重"/"中等"
 */
void AlarmEngine::triggerAlarm(const QString &msg, const QString &level) {
    AlarmRecord record;
    record.id = ++m_alarmCounter;// 先自增，赋值给报警id，每条报警id唯一
    record.message = msg;// 报警文字
    record.level = level;// 报警等级
    // 获取当前系统时间，格式 HH:mm:ss（时:分:秒）
    record.time = QDateTime::currentDateTime().toString("HH:mm:ss");
    // 发射信号，外部接收这条报警记录
    emit newAlarmTriggered(record);
}