#include "zonemodel.h"
#include <QDebug>
#include <cmath>

/**
 * @brief ZoneModel 构造函数
 * @note 初始化 4 个区域的默认状态。
 * 这些默认值会在程序启动时短暂显示，随后会被 MainWindow 传进来的真实/缓存数据覆盖。
 */
ZoneModel::ZoneModel(QObject *parent) : QAbstractListModel(parent) {
    for(int i = 1; i <= 4; ++i) {
        ZoneData z;
        z.zoneId = i;
        z.zoneName = (i==1)?"开放办公区": ((i==2)?"会议室":((i==3)?"经理办公室":"走廊/大堂"));
        z.currentTemp = 26.0f + i * 0.5f;
        z.setTemp = 24.0f;
        z.userSetTemp = 24.0f;
        z.valveOpening = 0.0f;
        z.co2Level = 600;
        z.isOccupied = (i % 2 != 0); // 奇数区有人，偶数区无人（与UI一致）
        z.isPowerOn = true;
        z.mode = ACMode::Cooling;
        z.freshAirOpening = 10.0f;
        m_zones.append(z);
    }
}

/**
 * @brief 检查是否所有区域都处于开机状态
 * @note 供 QML 顶部的“全局总开关”读取状态使用。
 */

bool ZoneModel::allPowerOn() const {
    for (const auto &z : m_zones) {
        if (!z.isPowerOn) return false;
    }
    return true;
}

/**
 * @brief 返回区域总数
 */
int ZoneModel::rowCount(const QModelIndex &parent) const {
    return parent.isValid() ? 0 : m_zones.size();
}

/**
 * @brief 将 C++ 数据喂给 QML 界面的核心函数
 * @note QML 里的 currentTemp、zoneName 等变量，就是通过这个 switch 读取到的。
 */
QVariant ZoneModel::data(const QModelIndex &index, int role) const {
    if (!index.isValid() || index.row() >= m_zones.size()) return QVariant();
    const ZoneData &zone = m_zones[index.row()];
    switch (role) {
    case ZoneIdRole: return zone.zoneId;
    case ZoneNameRole: return zone.zoneName;
    // 浮点数统一保留 1 位小数，避免 UI 显示 24.0000001 这种长串
    case CurrentTempRole: return QString::number(zone.currentTemp, 'f', 1);
    case TargetTempRole: return QString::number(zone.setTemp, 'f', 1);
    case UserSetTempRole: return QString::number(zone.userSetTemp, 'f', 1);
    // 阀门和新风开度不要小数，保留整数
    case ValveOpeningRole: return QString::number(zone.valveOpening, 'f', 0);
    case Co2LevelRole: return static_cast<int>(zone.co2Level);
    // 舒适度判断逻辑：26度以下算舒适（你可以根据实际需要调整）
    case ComfortStatusRole: return (zone.currentTemp <= 26.0f) ? "舒适" : "偏暖";
    case ModeRole: return (zone.mode == ACMode::Cooling) ? "制冷" : "制热";
    case IsPowerOnRole: return zone.isPowerOn;
    case FreshAirRole: return QString::number(zone.freshAirOpening, 'f', 0);
    default: return QVariant();
    }
}

/**
 * @brief 将 C++ 的枚举映射为 QML 可以识别的字符串键名
 */
QHash<int, QByteArray> ZoneModel::roleNames() const {
    QHash<int, QByteArray> roles;
    roles[ZoneIdRole] = "zoneId";
    roles[ZoneNameRole] = "zoneName";
    roles[CurrentTempRole] = "currentTemp";
    roles[TargetTempRole] = "targetTemp";
    roles[UserSetTempRole] = "userSetTemp";
    roles[ValveOpeningRole] = "valveOpening";
    roles[Co2LevelRole] = "co2Level";
    roles[ComfortStatusRole] = "comfortStatus";
    roles[ModeRole] = "modeStr";
    roles[IsPowerOnRole] = "isPowerOn";
    roles[FreshAirRole] = "freshAir";
    return roles;
}

/**
 * @brief 每秒一次的主循环更新（由 MainWindow 调用）
 * @note 【极其重要的细节】：
 * 这里故意没有把 UserSetTempRole（用户设定温度）包含进 dataChanged 中。
 * 原因：如果每秒都强制刷新设定温度，用户在输入框打字时会被强行打断。
 */
void ZoneModel::updateZones(const QList<ZoneData> &zones) {
    m_zones = zones;

    emit dataChanged(index(0), index(m_zones.size() - 1), {
        CurrentTempRole,
        TargetTempRole,
        ValveOpeningRole,
        Co2LevelRole,
        ComfortStatusRole,
        FreshAirRole,
        IsPowerOnRole,  // 强制刷新开关按钮
        ModeRole        // 强制刷新制冷/制热标签
    });
    emit allPowerStateChanged();
}

/**
 * @brief 专门用于接收 Modbus 里的设定温度，并推给 QML 更新
 * @note 加了 0.1f 的防抖死区。如果上位机改的温度和我们本地只差 0.05 度，不触发 UI 刷新，节省性能。
 */
void ZoneModel::syncSetTempFromModbus(int zoneId, float newTemp) {
    for (int i = 0; i < m_zones.size(); ++i) {
        if (m_zones[i].zoneId == zoneId) {
            if (std::abs(m_zones[i].userSetTemp - newTemp) > 0.1f) {
                m_zones[i].userSetTemp = newTemp;
                emit dataChanged(index(i), index(i), {UserSetTempRole, TargetTempRole});
            }
            break;
        }
    }
}

/**
 * @brief QML 端点击“下发指令”修改温度时触发
 * @note 先更新本地 UI，再发射信号通知 MainWindow 去写 Modbus 寄存器。
 */
void ZoneModel::setTargetTemp(int zoneId, float newTemp) {
    for (int i = 0; i < m_zones.size(); ++i) {
        if (m_zones[i].zoneId == zoneId) {
            m_zones[i].userSetTemp = newTemp;
            emit dataChanged(index(i), index(i), {UserSetTempRole, TargetTempRole});
            break;
        }
    }
    // 发射信号给 MainWindow.cpp 处理底层通信
    emit requestTempChange(zoneId, newTemp);
}

/**
 * @brief QML 端切换“制冷/制热”模式时触发
 */
void ZoneModel::setAcMode(int zoneId, int modeIndex) {
    for (int i = 0; i < m_zones.size(); ++i) {
        if (m_zones[i].zoneId == zoneId) {
            // 0 代表制冷，1 代表制热
            m_zones[i].mode = (modeIndex == 0) ? ACMode::Cooling : ACMode::Heating;
            emit dataChanged(index(i), index(i), {ModeRole, TargetTempRole});
            break;
        }
    }
    emit requestModeChange(zoneId, modeIndex);
}

/**
 * @brief QML 端点击某个区域的独立开关时触发
 */
void ZoneModel::setZonePower(int zoneId, bool isOn) {
    for (int i = 0; i < m_zones.size(); ++i) {
        if (m_zones[i].zoneId == zoneId) {
            m_zones[i].isPowerOn = isOn;
            // 通知该卡片刷新电源状态
            emit dataChanged(index(i), index(i), {IsPowerOnRole});
            // 通知全局开关刷新（因为部分关可能变成非全关）
            emit allPowerStateChanged();
            break;
        }
    }
    emit requestPowerStateChange(zoneId, isOn);
}

/**
 * @brief QML 端点击顶部“全局总开关”时触发
 */
void ZoneModel::setAllZonesPower(bool isOn) {
    for (int i = 0; i < m_zones.size(); ++i) {
        m_zones[i].isPowerOn = isOn;
        // 给 MainWindow 发送每一个区域的改变信号
        emit requestPowerStateChange(m_zones[i].zoneId, isOn);
    }
    // 一次性通知 UI 所有区域的卡片全部刷新
    emit dataChanged(index(0), index(m_zones.size() - 1), {IsPowerOnRole});
    emit allPowerStateChanged();
}

/**
 * @brief 切换全局开关（全开 -> 全关，或 全关 -> 全开）
 */
void ZoneModel::toggleAllZonesPower() {
    setAllZonesPower(!allPowerOn());
}