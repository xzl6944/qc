#include "modbusmaster.h"
#include <QVariant>
#include <QDebug>

ModbusMaster::ModbusMaster(QObject *parent)
    : QObject(parent), m_modbusClient(new QModbusTcpClient(this)) // 移除 m_currentReply 初始化
{
    connect(m_modbusClient, &QModbusClient::errorOccurred, this, &ModbusMaster::handleError);
}

ModbusMaster::~ModbusMaster()
{
    if (m_modbusClient) m_modbusClient->disconnectDevice();
}

void ModbusMaster::connectToHost(const QString &host, int port)
{
    if (m_modbusClient->state() == QModbusDevice::ConnectedState) return;
    m_modbusClient->setConnectionParameter(QModbusDevice::NetworkPortParameter, port);
    m_modbusClient->setConnectionParameter(QModbusDevice::NetworkAddressParameter, host);
    m_modbusClient->setTimeout(1000);
    m_modbusClient->setNumberOfRetries(3);

    if (m_modbusClient->connectDevice()) {
        qDebug() << "[Modbus] 正在连接虚拟服务器:" << host << ":" << port;
        emit connected();
    } else {
        emit errorOccurred(m_modbusClient->errorString());
    }
}

/**
 * @brief 读取保持寄存器（功能码03）—— 使用原子变量防雪崩
 */
void ModbusMaster::readHoldingRegisters(int slaveId, int startAddress, int numberOfEntries)
{
    if (!m_modbusClient || m_modbusClient->state() != QModbusDevice::ConnectedState) return;

    // 【核心修改】使用原子操作实现无锁并发拦截
    // 如果 m_isReading 是 false，把它改为 true 并继续执行；如果已经是 true，说明上一次没完成，直接返回
    bool expected = false;
    if (!m_isReading.compare_exchange_strong(expected, true)) {
        return; // 上一次请求还没完成，直接跳过本次轮询
    }

    QModbusDataUnit readUnit(QModbusDataUnit::HoldingRegisters, startAddress, numberOfEntries);

    if (auto *reply = m_modbusClient->sendReadRequest(readUnit, slaveId)) {
        if (!reply->isFinished()) {
            // 异步应答完成后触发 readReady，不用保存 reply 指针
            connect(reply, &QModbusReply::finished, this, &ModbusMaster::readReady);
        } else {
            // 极端情况：立刻完成。必须立刻释放锁！
            m_isReading.store(false);
            delete reply;
        }
    } else {
        // 如果请求发送失败（比如总线忙），也必须立刻释放锁，否则系统就永久卡死了
        m_isReading.store(false);
    }
}

/**
 * @brief 读请求完成回调槽函数
 */
void ModbusMaster::readReady()
{
    // 通过 sender() 获取是哪个应答对象完成
    auto reply = qobject_cast<QModbusReply *>(sender());
    if (!reply) {
        m_isReading.store(false); // 安全兜底，防止异常导致锁未释放
        return;
    }

    if (reply->error() == QModbusDevice::NoError) {
        emit registersRead(reply->result());
    } else {
        emit errorOccurred(reply->errorString());
    }

    // 安全回收应答对象
    reply->deleteLater();

    // 【核心修改】收到应答或报错后，立刻释放原子锁，允许下一次轮询
    m_isReading.store(false);
}

void ModbusMaster::handleError(QModbusDevice::Error error)
{
    if (error == QModbusDevice::NoError) return;
    emit errorOccurred(m_modbusClient->errorString());
}

void ModbusMaster::disconnectDevice()
{
    m_modbusClient->disconnectDevice();
}

void ModbusMaster::writeSingleRegister(int slaveId, int address, int value)
{
    if (!m_modbusClient || m_modbusClient->state() != QModbusDevice::ConnectedState) return;

    QModbusDataUnit writeUnit(QModbusDataUnit::HoldingRegisters, address, 1);
    writeUnit.setValue(0, value);

    if (auto *reply = m_modbusClient->sendWriteRequest(writeUnit, slaveId)) {
        if (!reply->isFinished()) {
            connect(reply, &QModbusReply::finished, reply, &QModbusReply::deleteLater);
        } else {
            reply->deleteLater();
        }
    }
}