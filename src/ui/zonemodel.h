// zonemodel.h
#ifndef ZONEMODEL_H
#define ZONEMODEL_H
#include <QAbstractListModel>
#include <QList>
#include "types.h"

class ZoneModel : public QAbstractListModel {
    Q_OBJECT
    // 【有用】用来给界面显示“全开/全关”的开关状态
    Q_PROPERTY(bool allPowerOn READ allPowerOn NOTIFY allPowerStateChanged)

public:
    explicit ZoneModel(QObject *parent = nullptr);
    bool allPowerOn() const;

    // QML 读取的字段
    enum ZoneRoles {
        ZoneIdRole = Qt::UserRole + 1,
        ZoneNameRole, CurrentTempRole, TargetTempRole, UserSetTempRole,
        ValveOpeningRole, Co2LevelRole, ComfortStatusRole, ModeRole,
        IsPowerOnRole, FreshAirRole
    };

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    QHash<int, QByteArray> roleNames() const override;

public slots:
    // 核心更新函数（主循环每秒调用）
    void updateZones(const QList<ZoneData> &zones);

    // 供 QML 按钮点击时调用的函数
    Q_INVOKABLE void setTargetTemp(int zoneId, float newTemp);
    Q_INVOKABLE void setAcMode(int zoneId, int modeIndex);
    Q_INVOKABLE void setZonePower(int zoneId, bool isOn);
    Q_INVOKABLE void setAllZonesPower(bool isOn);
    Q_INVOKABLE void toggleAllZonesPower();
    Q_INVOKABLE void syncSetTempFromModbus(int zoneId, float newTemp);

signals:
    // 发送给 MainWindow 处理的信号
    void requestTempChange(int zoneId, float newTemp);
    void requestModeChange(int zoneId, int modeIndex);
    void requestPowerStateChange(int zoneId, bool isOn);
    void allPowerStateChanged();

private:
    QList<ZoneData> m_zones;
};
#endif