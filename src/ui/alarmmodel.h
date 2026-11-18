// alarmmodel.h
#ifndef ALARMMODEL_H
#define ALARMMODEL_H

#include <QAbstractListModel>
#include "alarmengine.h"

class AlarmModel : public QAbstractListModel {
    Q_OBJECT
public:
    explicit AlarmModel(QObject *parent = nullptr);

    // 【核心】定义 QML 可以读取的三个角色（对应 UI 的列）
    enum AlarmRoles {
        TimeRole = Qt::UserRole + 1, // 时间
        MessageRole,                 // 报警内容
        LevelRole                    // 报警等级
    };

    // 重写基类方法，行数、单行数据、角色名称
    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    QHash<int, QByteArray> roleNames() const override;

public slots:
    // 供 C++ 端（MainWindow）调用的槽函数，用于插入新报警
    void addAlarm(const AlarmRecord &record);

private:
    QList<AlarmRecord> m_alarms; // 底层存储容器
};
#endif