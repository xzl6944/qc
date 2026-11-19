#include "roommodel.h"
#include <QRandomGenerator>
#include <cmath>

RoomModel::RoomModel(QObject *parent) : QObject(parent)
{

}

void RoomModel::simulateStep(QList<ZoneData> &zones)
{
    // 遍历每一个房间/区域，逐个做物理仿真
    for (int i = 0; i < zones.size(); ++i)
    {
        // 使用引用，直接修改列表内原始对象，不需要拷贝
        ZoneData &z = zones[i];

        // 判断空调模式：是否制冷模式
        bool isCooling = (z.mode == ACMode::Cooling);
        // 判断空调是否开机
        bool isPowerOn = z.isPowerOn;

        // 室外环境温度：制冷模式室外32℃；制热模式室外‑5℃
        float zoneOutdoorTemp = isCooling ? 32.0f : -5.0f;

        // ========== 1. 新风开度自动控制逻辑 ==========
        float freshAirOpening = 10.0f; // 默认新风开度10%
        if (z.co2Level > 1000)
        {
            // CO₂大于1000ppm，新风全开100%，强力通风换气
            freshAirOpening = 100.0f;
        }
        else if (z.co2Level > 800)
        {
            // CO₂在800~1000区间，线性插值：随CO₂升高逐步开大新风
            // 800ppm →10%；1000ppm→100%
            freshAirOpening = 10.0f + ((z.co2Level - 800) / 200.0f) * 90.0f;
        }
        z.freshAirOpening = freshAirOpening; // 将计算得到的新风开度写回区域数据


        // ========== 2. CO₂浓度仿真计算 ==========
        // 人员产生CO₂：有人占用时，随机产生0.5~1.5单位CO₂；无人则无生成
        float generation = z.isOccupied ? QRandomGenerator::global()->bounded(5, 15) / 10.0f : 0.0f;

        // 新风稀释CO₂：高于室外基准400ppm才会被新风稀释；新风开度越大稀释越强
        float dilution = (z.co2Level > 400) ? (z.co2Level - 400) * 0.02f * (freshAirOpening / 100.0f) : 0.0f;

        // CO₂ = 当前值 + 人员产生量 − 新风稀释带走量
        z.co2Level += generation - dilution;

        // CO₂上下限钳位：最低室外400ppm，最高不超过1500ppm
        if (z.co2Level < 400)
            z.co2Level = 400;
        if (z.co2Level > 1500)
            z.co2Level = 1500;


        // ========== 3. 房间热平衡，温度仿真 ==========
        // 墙体传热：室内外温差带来的墙体换热，系数0.005
        // 室外温度高于室内：热量传入房间；室外低于室内：热量流出房间
        float heatTransfer = (zoneOutdoorTemp - z.currentTemp) * 0.005f;

        // 新风带入热量：引入室外新风会改变室内温度；新风开度越大影响越大，系数0.003
        float freshAirHeat = (zoneOutdoorTemp - z.currentTemp) * 0.003f * (freshAirOpening / 100.0f);

        // HVAC空调出力效果，阀门开度决定空调功率，关机则出力为0
        float hvacEffect = 0.0f;
        if (isPowerOn)
        {
            if (isCooling)
            {
                // 制冷模式：阀门全开(100%)最大制冷系数0.8
                hvacEffect = (z.valveOpening / 100.0f) * 0.8f;
            }
            else
            {
                // 制热模式：阀门全开(100%)最大制热系数1.0，制热能力略强于制冷
                hvacEffect = (z.valveOpening / 100.0f) * 1.0f;
            }
        }

        float newTemp = z.currentTemp;
        if (isCooling)
        {
            // 制冷模式：墙体传热 + 新风热影响 − 空调制冷带走热量
            newTemp = z.currentTemp + heatTransfer + freshAirHeat - hvacEffect;
        }
        else
        {
            // 制热模式：墙体传热 + 新风热影响 + 空调制热增加热量
            newTemp = z.currentTemp + heatTransfer + freshAirHeat + hvacEffect;
        }

        // 温度钳位，限制房间仿真温度范围16~35℃，防止计算出现极端值
        if (newTemp < 16.0f)
            newTemp = 16.0f;
        if (newTemp > 32.0f)
            newTemp = 32.0f;

        // 更新房间当前温度
        z.currentTemp = newTemp;
    }
}
