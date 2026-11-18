#ifndef CHARTMODULE_H
#define CHARTMODULE_H

#include <QObject>
#include <QVariantList>

class ChartModule : public QObject {
    Q_OBJECT
    // 【已精简】只保留 QML 画图需要的 tempHistory
    Q_PROPERTY(QVariantList tempHistory READ tempHistory NOTIFY dataChanged)

public:
    explicit ChartModule(QObject *parent = nullptr);

    // 【已精简】只保留唯一需要的读取函数
    QVariantList tempHistory() const { return m_tempHistory; }

public slots:
    void appendData(float t1, float tar1, float t2, float tar2, float t3, float tar3, float t4, float tar4, float power);

signals:
    void dataChanged();

private:
    QVariantList m_tempHistory;
};

#endif // CHARTMODULE_H