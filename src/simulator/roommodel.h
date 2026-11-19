#ifndef ROOMMODEL_H
#define ROOMMODEL_H

#include <QObject>
#include <QList>
#include "types.h"

class RoomModel : public QObject
{
    Q_OBJECT
public:
    explicit RoomModel(QObject *parent = nullptr);

    // 【核心接口】传入当前所有区域的状态，物理引擎计算并直接修改其中的温度和CO2
    void simulateStep(QList<ZoneData> &zones);

private:
    // 可以保留一些内部变量，比如外部扰动等
};

#endif // ROOMMODEL_H