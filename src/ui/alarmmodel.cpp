// alarmmodel.cpp
#include "alarmmodel.h"

AlarmModel::AlarmModel(QObject *parent) : QAbstractListModel(parent) {}

int AlarmModel::rowCount(const QModelIndex &parent) const {
    if (parent.isValid()) return 0; // 树形结构才需要，列表返回 0 即可
    return m_alarms.size();
}

QVariant AlarmModel::data(const QModelIndex &index, int role) const {
    if (!index.isValid() || index.row() >= m_alarms.size()) return QVariant();
    const AlarmRecord &rec = m_alarms[index.row()];
    // 根据 Role，把数据喂给 QML
    switch (role) {
    case TimeRole: return rec.time;
    case MessageRole: return rec.message;
    case LevelRole: return rec.level;
    default: return QVariant();
    }
}

QHash<int, QByteArray> AlarmModel::roleNames() const {
    QHash<int, QByteArray> roles;
    // QML 里写 text: time，就是在这里绑定的
    roles[TimeRole] = "time";
    roles[MessageRole] = "message";
    roles[LevelRole] = "level";
    return roles;
}

void AlarmModel::addAlarm(const AlarmRecord &record) {
    // 【规范写法】必须成对调用 begin/end，通知 QML 刷新
    beginInsertRows(QModelIndex(), 0, 0);
    m_alarms.prepend(record); // 最新报警插在最前面
    endInsertRows();

    // 【防内存溢出保护】最多保留 50 条，删掉最旧的
    if (m_alarms.size() > 50) {
        beginRemoveRows(QModelIndex(), 50, m_alarms.size() - 1);
        while (m_alarms.size() > 50) m_alarms.removeLast();
        endRemoveRows();
    }
}