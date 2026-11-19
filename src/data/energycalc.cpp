#include "energycalc.h"
#include <QDebug>

/**
 * @brief EnergyCalc 构造函数，初始化全部统计变量
 * @param parent Qt父对象
 */
EnergyCalc::EnergyCalc(QObject *parent)
    : QObject(parent),
    m_totalKwh(0),         // 总耗电量初始0
    m_savedKwh(0),         // 节约电量初始0
    m_co2Reduction(0),    // CO₂减排初始0
    m_currentPower(0),    // 当前功率初始0
    m_efficiencyScore(100)// 初始能效满分100
{
}

/**
 * @brief updateEnergy 能耗统计更新，仿真每一步调用
 * @param powerKw 空调当前功率 kW
 * @param durationSeconds 当前功率持续时间 秒
 * 逻辑：
 * 1. 计算本时间片消耗电能，累加到总耗电
 * 2. 当空调运行时，按30%计算相比定频空调的节电量
 * 3. 根据节电量换算CO₂减排
 * 4. 根据负载率计算动态能效评分
 * 5. 发射信号通知UI刷新
 */
void EnergyCalc::updateEnergy(float powerKw, float durationSeconds)
{
    // 保存瞬时功率，用于UI显示
    m_currentPower = powerKw;

    // 秒转换为小时：kWh = kW * h
    float hours = durationSeconds / 3600.0f;
    // 计算这一个仿真步实际消耗多少度电
    float currentKwh = powerKw * hours;

    // 累加总实际耗电量
    m_totalKwh += currentKwh;

    // 【核心模型】：仅设备功率>0（空调正在运行）才计算节约电量
    // 模型假设：本系统对比老旧定频空调，综合节能30%
    if (powerKw > 0.0f)
    {
        // 本次仿真步节约电量 = 本次实际耗电 × 30%
        float savedThisTime = currentKwh * 0.30f;
        m_savedKwh += savedThisTime;

        // 火电排放因子：每节约1kWh电，减少0.785kg CO₂排放
        m_co2Reduction += (savedThisTime * 0.785f);
    }

    // -------- 动态能效评分计算 --------
    // 空调额定最大功率，单位kW
    float maxRatedPower = 5.4f;
    // 负载率 = 当前功率 / 额定最大功率
    float ratio = powerKw / maxRatedPower;

    if (powerKw < 0.1f)
    {
        // 功率接近0，空调停机，能效直接满分100
        m_efficiencyScore = 100;
    }
    else
    {
        // 负载越高，分数越低；最大负载扣40分
        m_efficiencyScore = 100 - static_cast<int>(ratio * 40);

        // 分数钳位，保证不会超过100，最低不低于50分
        if (m_efficiencyScore > 100)
            m_efficiencyScore = 100;
        if (m_efficiencyScore < 50)
            m_efficiencyScore = 50;
    }

    // 发射信号，QML界面所有绑定属性刷新
    emit dataChanged();
}
