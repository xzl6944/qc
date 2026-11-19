#ifndef ENERGYCALC_H
#define ENERGYCALC_H
#include <QObject>
#include <QString>

/**
 * @brief EnergyCalc 能耗统计模块
 * 统计空调总耗电、节电量、CO₂减排、实时功率、能效评分；属性直接供QML绑定UI显示
 */
class EnergyCalc : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QString totalKwhStr READ totalKwhStr NOTIFY dataChanged)
    Q_PROPERTY(QString savedKwhStr READ savedKwhStr NOTIFY dataChanged)
    Q_PROPERTY(QString co2ReductionStr READ co2ReductionStr NOTIFY dataChanged)
    Q_PROPERTY(QString currentPowerStr READ currentPowerStr NOTIFY dataChanged)
    Q_PROPERTY(int efficiencyScore READ efficiencyScore NOTIFY dataChanged)

public:
    explicit EnergyCalc(QObject *parent = nullptr);

    // @return 格式化总耗电量字符串，例："0.000 kWh"
    QString totalKwhStr() const { return QString::number(m_totalKwh, 'f', 3) + " kWh"; }
    // @return 格式化节约电量字符串
    QString savedKwhStr() const { return QString::number(m_savedKwh, 'f', 3) + " kWh"; }
    // @return 格式化CO₂减排字符串
    QString co2ReductionStr() const { return QString::number(m_co2Reduction, 'f', 3) + " kg"; }
    // @return 格式化实时功率字符串
    QString currentPowerStr() const { return QString::number(m_currentPower, 'f', 2) + " kW"; }
    // @return 能效评分，取值范围50~100
    int efficiencyScore() const { return m_efficiencyScore; }

    // C++侧读取原始数值（不带单位）
    float getTotalKwh() const { return m_totalKwh; }
    float getSavedKwh() const { return m_savedKwh; }
    float getCo2Reduction() const { return m_co2Reduction; }

    /**
     * @brief updateEnergy 能耗更新入口，每仿真步调用
     * @param powerKw 当前空调功率(kW)
     * @param durationSeconds 本时间片持续时长(秒)
     */
    void updateEnergy(float powerKw, float durationSeconds);

signals:
    // 统计数据更新，通知QML刷新界面
    void dataChanged();

private:
    float m_totalKwh;     // 累计实际总耗电量 kWh
    float m_savedKwh;     // 累计节约电量 kWh（经验30%节能模型）
    float m_co2Reduction; // 累计CO₂减排量 kg
    float m_currentPower; // 当前瞬时功率 kW
    int m_efficiencyScore;// 动态能效评分 50‑100
};

#endif // ENERGYCALC_H
