#include "chartmodule.h"
#include <QVariantMap>

// 【已精简】移除了构造函数里多余的初始化
ChartModule::ChartModule(QObject *parent) : QObject(parent) {}

void ChartModule::appendData(float t1, float tar1, float t2, float tar2, float t3, float tar3, float t4, float tar4, float power) {
    // 【已精简】移除了无用的 m_tickCount++
    QVariantMap map;
    map["t1"] = t1; map["tar1"] = tar1;
    map["t2"] = t2; map["tar2"] = tar2;
    map["t3"] = t3; map["tar3"] = tar3;
    map["t4"] = t4; map["tar4"] = tar4;
    map["power"] = power;

    m_tempHistory.append(map);
    // 限制最大 60 个点（60秒数据），滑动窗口机制
    if (m_tempHistory.size() > 60) m_tempHistory.removeFirst();
    emit dataChanged();
}