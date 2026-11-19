#ifndef MODBUSMASTER_H
#define MODBUSMASTER_H

#include <QObject>
#include <QModbusTcpClient>
#include <QModbusDataUnit>
#include <QModbusReply>
#include <atomic> // 【新增】引入原子操作库

class ModbusMaster : public QObject
{
    Q_OBJECT
public:
    explicit ModbusMaster(QObject *parent = nullptr);
    ~ModbusMaster();

    void connectToHost(const QString &host, int port = 502);
    void disconnectDevice();
    void readHoldingRegisters(int slaveId, int startAddress, int numberOfEntries);
    void writeSingleRegister(int slaveId, int address, int value);

signals:
    void registersRead(const QModbusDataUnit &unit);
    void connected();
    void errorOccurred(const QString &errorMsg);

private slots:
    void readReady();
    void handleError(QModbusDevice::Error error);

private:
    QModbusTcpClient *m_modbusClient;

    // 【修改】使用原子布尔变量作为“单请求在途”标志位锁，防止并发冲突
    std::atomic<bool> m_isReading{false};
};

#endif // MODBUSMASTER_H