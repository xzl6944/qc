#ifndef STRATEGYENGINE_H
#define STRATEGYENGINE_H

#include <QTime>
#include <QString>

class StrategyEngine {
public:
    StrategyEngine();

    // 1. 计算最终目标温度 (分时段 + 占用检测)
    float calculateTargetTemp(float userSetTemp, bool isOccupied, QTime currentTime);

    // 2. 夜间通风预冷判断 (文档 5.2)
    bool checkPreCooling(float indoorTemp, float outdoorTemp, QTime currentTime);

    // 3. 提前停机策略 (文档 5.2)
    bool checkEarlyShutdown(QTime currentTime, float currentTemp, float targetTemp);

    // 4. 新风需求控制 - 基于 CO2 浓度 (文档 5.5)
    float calculateFreshAirOpening(int co2Level);

    // 5. 【新增】新风需求控制 - 基于焓值比较 (文档 5.5)
    float calculateEconomizerOpening(float indoorTemp, float indoorHum, float outdoorTemp, float outdoorHum);

    // 增加 isCooling 参数，让策略引擎知道怎么偏
    float calculateTargetTemp(float userSetTemp, bool isOccupied, QTime currentTime, bool isCooling);
};

#endif // STRATEGYENGINE_H