#include "strategyengine.h"
#include <cmath>

StrategyEngine::StrategyEngine() {}

float StrategyEngine::calculateTargetTemp(float userSetTemp, bool isOccupied, QTime currentTime, bool isCooling) {
    float targetTemp = userSetTemp;
    int hour = currentTime.hour();

    if (isOccupied) {
        // =========================================================
        // 情况A：有人在，严格执行【分时段策略】的舒适区限制
        // =========================================================
        if (hour >= 8 && hour < 18) {
            // 工作时段 (08:00 - 18:00)
            if (isCooling) {
                // 制冷舒适区：24-26℃
                if (targetTemp < 24.0f) targetTemp = 24.0f;
                if (targetTemp > 26.0f) targetTemp = 26.0f;
            } else {
                // 制热工作时段舒适区：18-24℃
                if (targetTemp < 18.0f) targetTemp = 18.0f;
                if (targetTemp > 24.0f) targetTemp = 24.0f;
            }
        } else {
            // 夜间节能时段 (18:00 - 08:00)
            if (isCooling) {
                // 制冷夜间节能区：26-28℃
                if (targetTemp < 26.0f) targetTemp = 26.0f;
                if (targetTemp > 28.0f) targetTemp = 28.0f;
            } else {
                // 制热夜间节能区：16-18℃
                if (targetTemp < 16.0f) targetTemp = 16.0f;
                if (targetTemp > 18.0f) targetTemp = 18.0f;
            }
        }
    } else {
        // =========================================================
        // 情况B：无人，【占用检测联动绝对优先】，允许突破所有时段限制
        // =========================================================
        if (isCooling) {
            targetTemp += 3.0f; // 夏季无人：你设定20，策略直接变成23
        } else {
            targetTemp -= 3.0f; // 冬季无人：你设定28，策略直接变成25
        }
    }

    // =========================================================
    // 仅保留绝对物理底线保护（防止出现极端离谱的温度）
    // =========================================================
    if (targetTemp > 32.0f) targetTemp = 32.0f;
    if (targetTemp < 16.0f) targetTemp = 16.0f;

    return targetTemp;
}

// =========================================================================
// 以下为预留的高级节能策略接口 (目前主流程未调用，专为二期升级预留)
// =========================================================================

/**
 * @brief 夜间通风预冷判断
 * @note 工业级优化：增加了室外温度下限保护，防止夜间过冷导致水管冻结或极度不适
 */
bool StrategyEngine::checkPreCooling(float indoorTemp, float outdoorTemp, QTime currentTime) {
    int hour = currentTime.hour();

    // 仅在夜间 00:00 - 06:00 评估
    if (hour >= 0 && hour <= 6) {
        // 必须满足：室外温度比室内低 2℃ 以上，且室外温度不能低于 10℃（防止冻管）
        if (outdoorTemp < (indoorTemp - 2.0f) && outdoorTemp > 10.0f) {
            return true;
        }
    }
    return false;
}

/**
 * @brief 提前停机策略
 * @note 工业级优化：精确到分钟的边界判断，利用建筑热惰性提前关闭主机
 */
bool StrategyEngine::checkEarlyShutdown(QTime currentTime, float currentTemp, float targetTemp) {
    int hour = currentTime.hour();
    int minute = currentTime.minute();

    // 下班前 17:00 - 17:30 的窗口期
    if (hour == 17 && minute <= 30) {
        // 仅当当前温度已经非常接近目标温度（温差绝对值 < 1.0℃），才允许提前停机
        if (std::abs(currentTemp - targetTemp) < 1.0f) {
            return true;
        }
    }
    return false;
}

/**
 * @brief 新风需求控制 - 基于焓值比较 (Economizer)
 * @note 工业级优化：1.加入了异常值拦截(NaN)；2.将原本简单的 0/100 开关控制，升级为平滑的【比例控制】
 */
float StrategyEngine::calculateEconomizerOpening(float indoorTemp, float indoorHum, float outdoorTemp, float outdoorHum) {
    // 1. 安全校验：防止传感器故障输入异常数据导致系统崩溃
    if (std::isnan(indoorTemp) || std::isnan(outdoorTemp) ||
        std::isnan(indoorHum)  || std::isnan(outdoorHum) ||
        indoorHum < 0.0f || outdoorHum < 0.0f) {
        return 0.0f; // 数据异常时，保守起见关闭新风
    }

    // 2. 计算湿空气焓值 (kJ/kg)
    auto calcEnthalpy = [](float T, float RH) {
        return 1.006f * T + RH * 0.001f * (2501.0f + 1.86f * T);
    };

    float hIn = calcEnthalpy(indoorTemp, indoorHum);
    float hOut = calcEnthalpy(outdoorTemp, outdoorHum);

    // 3. 核心判断：只有当室外空气焓值更低（更凉爽干燥），且温度适宜（低于26℃）时，才利用自然冷源
    if (hOut < hIn && outdoorTemp < 26.0f) {
        // 采用比例控制：焓值差越大，新风阀开度越大
        float deltaH = hIn - hOut;
        // 假设室内外焓差达到 20 kJ/kg 时，新风全开(100%)
        float opening = (deltaH / 20.0f) * 100.0f;

        // 限制在 0 - 100 的合理区间内
        return std::max(0.0f, std::min(100.0f, opening));
    }

    return 0.0f; // 不满足条件，关闭新风经济器模式
}