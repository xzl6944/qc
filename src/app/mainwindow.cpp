#include "mainwindow.h"
#include "modbusmaster.h"
#include "pidcontroller.h"
#include "strategyengine.h"
#include "datalogger.h"
#include "zonemodel.h"
#include "alarmmodel.h"
#include "chartmodule.h"
#include "alarmengine.h"
#include "energycalc.h"
#include "roommodel.h"

#include <QTimer>
#include <QDebug>
#include <QTime>
#include <QDate>
#include <QMap>
#include <QQuickWidget>
#include <QQmlContext>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QListWidget>
#include <QStackedWidget>
#include <QRandomGenerator>
#include <QDateTime>
#include <cmath>
#include <QElapsedTimer> // 用于通信断开的防抖

// 【新增】CSV导出相关的头文件
#include <QFile>
#include <QTextStream>
#include <QStandardPaths>

// ========== 全局静态缓存 ==========
static float g_userSetTempCache[4] = {25.0f, 24.5f, 23.0f, 29.0f};
static float g_exactTempCache[4] = {27.0f, 27.0f, 29.0f, 23.0f};
static float g_co2Cache[4] = {600.0f, 600.0f, 600.0f, 600.0f};
static ACMode g_zoneModes[4] = {ACMode::Cooling, ACMode::Cooling, ACMode::Cooling, ACMode::Heating};
static bool g_zonePowerStates[4] = {true, true, true, true};

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
{
    m_zoneModel = new ZoneModel(this);
    m_alarmModel = new AlarmModel(this);
    m_chartModel = new ChartModule(this);
    m_energyCalc = new EnergyCalc(this);
    m_logger = new DataLogger(this);
    m_roomSimulator = new RoomModel(this);

    QList<ZoneData> initZones;
    for(int i = 0; i < 4; ++i) {
        ZoneData z;
        z.zoneId = i + 1;
        z.zoneName = (z.zoneId==1)?"开放办公区": ((z.zoneId==2)?"会议室":((z.zoneId==3)?"经理办公室":"走廊/大堂"));
        z.currentTemp = g_exactTempCache[i];
        z.userSetTemp = g_userSetTempCache[i];
        z.setTemp = g_userSetTempCache[i];
        z.valveOpening = 0.0f;
        z.co2Level = g_co2Cache[i];
        z.isOccupied = (z.zoneId % 2 != 0);
        z.isPowerOn = true;
        z.mode = ACMode::Cooling;
        z.freshAirOpening = 10.0f;
        initZones.append(z);
    }
    m_zoneModel->updateZones(initZones);

    setupUi();

    m_modbus = new ModbusMaster(this);
    m_logger->initDatabase();
    m_strategy = new StrategyEngine();
    m_alarmEngine = new AlarmEngine(this);

    m_zonePids.clear();
    for(int i = 1; i <= 4; ++i) {
        PIDController *pid = new PIDController();
        pid->setParams(15.0f, 0.2f, 0.1f);
        m_zonePids.insert(i, pid);
    }
    m_currentZones.clear();

    // A. 报警引擎触发 -> 存入数据库并在 UI 报警列表顶部插入
    connect(m_alarmEngine, &AlarmEngine::newAlarmTriggered, this, [=](const AlarmRecord &rec){
        int lvl = (rec.level == "严重") ? 1 : ((rec.level == "中等") ? 2 : 3);
        m_logger->logAlarm(lvl, rec.message);
        m_alarmModel->addAlarm(rec);
    });

    // B. 用户通过 QML 修改设定温度
    connect(m_zoneModel, &ZoneModel::requestTempChange, this, [this](int zoneId, float newTemp){
        int addr = (zoneId - 1) * 4 + 1;
        m_modbus->writeSingleRegister(1, addr, static_cast<int>(newTemp * 10));
        g_userSetTempCache[zoneId - 1] = newTemp;
        for(int i=0; i<m_currentZones.size(); ++i) {
            if(m_currentZones[i].zoneId == zoneId) m_currentZones[i].userSetTemp = newTemp;
        }
        m_logger->saveDeviceConfig(m_currentZones);
    });

    // C. 用户切换制冷/制热模式
    connect(m_zoneModel, &ZoneModel::requestModeChange, this, [=](int zoneId, int modeIndex){
        g_zoneModes[zoneId - 1] = static_cast<ACMode>(modeIndex);
        if(m_zonePids.contains(zoneId)) m_zonePids.value(zoneId)->reset();
        for(int i=0; i<m_currentZones.size(); ++i) {
            if(m_currentZones[i].zoneId == zoneId) m_currentZones[i].mode = static_cast<ACMode>(modeIndex);
        }
        m_logger->saveDeviceConfig(m_currentZones);
    });

    // D. 用户开关机
    connect(m_zoneModel, &ZoneModel::requestPowerStateChange, this, [=](int zoneId, bool isOn){
        g_zonePowerStates[zoneId - 1] = isOn;
        if(!isOn && m_zonePids.contains(zoneId)) m_zonePids.value(zoneId)->reset();
        for(int i=0; i<m_currentZones.size(); ++i) {
            if(m_currentZones[i].zoneId == zoneId) m_currentZones[i].isPowerOn = isOn;
        }
        m_logger->saveDeviceConfig(m_currentZones);
    });

    // E. 1秒心跳定时器
    m_pollTimer = new QTimer(this);
    connect(m_pollTimer, &QTimer::timeout, this, [this]{
        m_modbus->readHoldingRegisters(1, 0, 16);
    });

    // F. Modbus 连接成功
    connect(m_modbus, &ModbusMaster::connected, this, [this]{
        if (auto *label = this->findChild<QLabel*>("statusLabel")) {
            label->setText("● 通信在线");
            label->setStyleSheet("color: #22c55e; font-size: 14px; font-family: 'Microsoft YaHei'; margin-right: 15px;");
        }
        for(int i = 0; i < 4; ++i) {
            int baseAddr = i * 4;
            m_modbus->writeSingleRegister(1, baseAddr + 0, static_cast<int>(g_exactTempCache[i] * 10));
            m_modbus->writeSingleRegister(1, baseAddr + 1, static_cast<int>(g_userSetTempCache[i] * 10));
            m_modbus->writeSingleRegister(1, baseAddr + 3, static_cast<int>(g_co2Cache[i]));
        }
        m_pollTimer->start(1000);
    });

    // G. Modbus 断开或异常
    connect(m_modbus, &ModbusMaster::errorOccurred, this, [this](const QString &err){
        Q_UNUSED(err);
        static QElapsedTimer errorTimer;
        if (!errorTimer.isValid() || errorTimer.elapsed() > 10000) {
            m_alarmEngine->checkSystemStatus(0, 25.0f, false);
            errorTimer.restart();
        }
        if (auto *label = this->findChild<QLabel*>("statusLabel")) {
            label->setText("● 通信断开");
            label->setStyleSheet("color: #ef4444; font-size: 14px; font-weight: bold; font-family: 'Microsoft YaHei'; margin-right: 15px;");
        }
        if (auto *alarmLight = this->findChild<QLabel*>("alarmLightLabel")) {
            alarmLight->setText("● 严重报警");
            alarmLight->setStyleSheet("color: #ef4444; font-size: 14px; font-weight: bold; font-family: 'Microsoft YaHei'; margin-right: 15px;");
        }
    });

    connect(m_modbus, &ModbusMaster::registersRead, this, &MainWindow::onModbusDataReceived);
    m_modbus->connectToHost("127.0.0.1", 1502);
}

MainWindow::~MainWindow() {}

void MainWindow::onModbusDataReceived(const QModbusDataUnit &unit)
{
    if (unit.valueCount() < 16) return;

    QTime now = QTime::currentTime();
    QList<ZoneData> uiZones;
    float totalPower = 0.0f;

    for (int i = 0; i < 4; ++i) {
        int id = i + 1;
        int baseAddr = i * 4;

        float currentTemp = g_exactTempCache[i];
        float modbusTemp = unit.value(baseAddr + 0) / 10.0f;
        if (modbusTemp >= 10.0f && modbusTemp <= 40.0f) {
            if (std::abs(modbusTemp - g_exactTempCache[i]) > 1.0f) {
                currentTemp = modbusTemp;
                g_exactTempCache[i] = modbusTemp;
            }
        }

        float modbusSetTemp = unit.value(baseAddr + 1) / 10.0f;
        if (modbusSetTemp >= 10.0f && modbusSetTemp <= 35.0f) {
            g_userSetTempCache[i] = modbusSetTemp;
            m_zoneModel->syncSetTempFromModbus(id, modbusSetTemp);
        }
        float setTemp = g_userSetTempCache[i];

        int modbusCo2 = unit.value(baseAddr + 3);
        if (modbusCo2 >= 400 && modbusCo2 <= 5000) {
            if (std::abs(modbusCo2 - g_co2Cache[i]) > 10.0f) {
                g_co2Cache[i] = modbusCo2;
            }
        }
        float co2 = g_co2Cache[i];

        bool isOccupied = (id % 2 != 0);
        bool isCooling = (g_zoneModes[i] == ACMode::Cooling);
        bool isPowerOn = g_zonePowerStates[i];

        float finalTargetTemp = m_strategy->calculateTargetTemp(setTemp, isOccupied, now, isCooling);

        float pidValveOutput = 0.0f;
        if (isPowerOn) {
            PIDController *pid = m_zonePids.value(id);
            if (isCooling) pidValveOutput = pid->calculate(finalTargetTemp, currentTemp, 1.0f);
            else pidValveOutput = pid->calculate(currentTemp, finalTargetTemp, 1.0f);
        }

        m_alarmEngine->checkSystemStatus(id, currentTemp, true);
        m_alarmEngine->checkAirQuality(id, co2);
        if (isPowerOn) m_alarmEngine->checkValveStuck(id, pidValveOutput, currentTemp);

        ZoneData zd;
        zd.zoneId = id;
        zd.zoneName = (id==1)?"开放办公区": ((id==2)?"会议室":((id==3)?"经理办公室":"走廊/大堂"));
        zd.currentTemp = currentTemp;
        zd.userSetTemp = setTemp;
        zd.setTemp = finalTargetTemp;
        zd.valveOpening = pidValveOutput;
        zd.co2Level = co2;
        zd.isOccupied = isOccupied;
        zd.isPowerOn = isPowerOn;
        zd.mode = g_zoneModes[i];
        uiZones.append(zd);

        float zonePower = isPowerOn ? (0.1f + (pidValveOutput / 100.0f) * 1.25f) : 0.0f;
        totalPower += zonePower;
    }

    m_roomSimulator->simulateStep(uiZones);

    for (int i = 0; i < 4; ++i) {
        int baseAddr = i * 4;
        g_exactTempCache[i] = uiZones[i].currentTemp;
        g_co2Cache[i] = uiZones[i].co2Level;

        m_modbus->writeSingleRegister(1, baseAddr + 0, static_cast<int>(uiZones[i].currentTemp * 10));
        m_modbus->writeSingleRegister(1, baseAddr + 1, static_cast<int>(uiZones[i].userSetTemp * 10));
        m_modbus->writeSingleRegister(1, baseAddr + 2, static_cast<int>(uiZones[i].valveOpening));
        m_modbus->writeSingleRegister(1, baseAddr + 3, static_cast<int>(uiZones[i].co2Level));
    }

    m_energyCalc->updateEnergy(totalPower, 1.0f);
    m_currentZones = uiZones;
    m_logger->logRealtimeZones(uiZones);
    m_logger->logTotalEnergy(m_energyCalc->getTotalKwh(), m_energyCalc->getSavedKwh(), m_energyCalc->getCo2Reduction(), totalPower);

    m_chartModel->appendData(
        uiZones[0].currentTemp, uiZones[0].setTemp,
        uiZones[1].currentTemp, uiZones[1].setTemp,
        uiZones[2].currentTemp, uiZones[2].setTemp,
        uiZones[3].currentTemp, uiZones[3].setTemp,
        totalPower
        );
    m_zoneModel->updateZones(uiZones);

    if (auto *label = this->findChild<QLabel*>("statusLabel")) {
        label->setText("● 通信在线");
        label->setStyleSheet("color: #22c55e; font-size: 14px; font-family: 'Microsoft YaHei'; margin-right: 15px;");
    }

    bool hasCritical = false;
    bool hasMedium = false;
    for (const auto &z : uiZones) {
        if (z.currentTemp < -50.0f || z.currentTemp > 90.0f) hasCritical = true;
        if (z.co2Level > 1200) hasCritical = true;
        else if (z.co2Level > 1000) hasMedium = true;
    }

    if (auto *alarmLight = this->findChild<QLabel*>("alarmLightLabel")) {
        if (hasCritical) {
            alarmLight->setText("● 严重报警");
            alarmLight->setStyleSheet("color: #ef4444; font-size: 14px; font-weight: bold; font-family: 'Microsoft YaHei'; margin-right: 15px;");
        } else if (hasMedium) {
            alarmLight->setText("● 中等报警");
            alarmLight->setStyleSheet("color: #f59e0b; font-size: 14px; font-weight: bold; font-family: 'Microsoft YaHei'; margin-right: 15px;");
        } else {
            alarmLight->setText("● 无报警");
            alarmLight->setStyleSheet("color: #64748B; font-size: 14px; font-family: 'Microsoft YaHei'; margin-right: 15px;");
        }
    }
}

QVariantMap MainWindow::queryHistory(const QString &date, int page, int pageSize) {
    return m_logger->getTotalEnergyHistory(date, page, pageSize);
}

// 【新增】导出 CSV 文件的实现
bool MainWindow::exportHistoryToCSV(const QString &date) {
    // 查出当天所有数据
    QVariantMap dataMap = m_logger->getTotalEnergyHistory(date, 1, 10000);
    QVariantList dataList = dataMap["dataList"].toList();

    if (dataList.isEmpty()) return false;

    // 生成路径：桌面
    QString fileName = "能耗历史导出_" + date + ".csv";
    QString filePath = QStandardPaths::writableLocation(QStandardPaths::DesktopLocation) + "/" + fileName;

    QFile file(filePath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) return false;

    QTextStream out(&file);
    // 【防乱码神技】写入 UTF-8 BOM 头，保证用 Excel 打开时中文不乱码
    out.setGenerateByteOrderMark(true);

    // 写表头（去掉了单位方便 Excel 计算）
    out << "时间,累计耗电量,今日节电量,CO2减排量,实时功率\n";

    // 遍历写入数据
    for (const auto &item : dataList) {
        QVariantMap row = item.toMap();
        QString time = row["time"].toString();
        QString totalKwh = row["total_kwh"].toString().replace(" kWh", "");
        QString savedKwh = row["saved_kwh"].toString().replace(" kWh", "");
        QString co2 = row["co2"].toString().replace(" kg", "");
        QString power = row["power"].toString().replace(" kW", "");

        out << time << "," << totalKwh << "," << savedKwh << "," << co2 << "," << power << "\n";
    }

    file.close();
    return true;
}

QWidget* MainWindow::createQmlTab(const QString& qmlPath, const QString& contextName, QObject* contextObject) {
    auto* quick = new QQuickWidget(this);
    quick->setResizeMode(QQuickWidget::SizeRootObjectToView);

    quick->rootContext()->setContextProperty("zoneModel", m_zoneModel);
    quick->rootContext()->setContextProperty("alarmModel", m_alarmModel);
    quick->rootContext()->setContextProperty("chartModel", m_chartModel);
    quick->rootContext()->setContextProperty("energyCalc", m_energyCalc);

    if (qmlPath.contains("achistoryUI")) {
        quick->rootContext()->setContextProperty("mainWindow", this);
    }

    if (!contextName.isEmpty()) quick->rootContext()->setContextProperty(contextName, contextObject);
    quick->setSource(QUrl(qmlPath));
    return quick;
}

void MainWindow::setupUi() {
    setWindowTitle("空调节能智控终端 - 4 区域工业级多模态演示");
    resize(1440, 900);

    auto* central = new QWidget(this);
    setCentralWidget(central);
    auto* mainVLayout = new QVBoxLayout(central);
    mainVLayout->setContentsMargins(0, 0, 0, 0);
    mainVLayout->setSpacing(0);

    auto* topBar = new QWidget(this);
    topBar->setFixedHeight(50);
    topBar->setStyleSheet("background-color: #1E293B; border-bottom: 1px solid #334155;");
    auto* topLayout = new QHBoxLayout(topBar);

    auto* titleLabel = new QLabel("空调节能智控平台", topBar);
    titleLabel->setStyleSheet("color: #38BDF8; font-size: 18px; font-weight: bold; font-family: 'Microsoft YaHei';");

    auto* statusLabel = new QLabel("● 通信连接中...", topBar);
    statusLabel->setObjectName("statusLabel");
    statusLabel->setStyleSheet("color: #f59e0b; font-size: 14px; font-family: 'Microsoft YaHei'; margin-right: 15px;");

    auto* alarmLightLabel = new QLabel("● 无报警", topBar);
    alarmLightLabel->setObjectName("alarmLightLabel");
    alarmLightLabel->setStyleSheet("color: #64748B; font-size: 14px; font-family: 'Microsoft YaHei'; margin-right: 15px;");

    m_timeLabel = new QLabel(topBar);
    m_timeLabel->setStyleSheet("color: #94A3B8; font-size: 16px; font-family: 'Microsoft YaHei';");

    topLayout->addWidget(titleLabel);
    topLayout->addStretch();
    topLayout->addWidget(statusLabel);
    topLayout->addWidget(alarmLightLabel);
    topLayout->addWidget(m_timeLabel);
    mainVLayout->addWidget(topBar);

    auto* contentWidget = new QWidget(this);
    auto* mainLayout = new QHBoxLayout(contentWidget);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->setSpacing(0);

    m_navList = new QListWidget(this);
    m_navList->setFixedWidth(200);
    m_navList->addItems({"主监控", "能耗分析", "历史数据", "报警记录", "参数设置"});

    m_stack = new QStackedWidget(this);
    m_stack->addWidget(createQmlTab("qrc:/acmonitorUI.qml", "zoneModel", m_zoneModel));
    m_stack->addWidget(createQmlTab("qrc:/acenergyUI.qml", "chartModel", m_chartModel));
    m_stack->addWidget(createQmlTab("qrc:/achistoryUI.qml", "chartModel", m_chartModel));
    m_stack->addWidget(createQmlTab("qrc:/acalarmUI.qml", "alarmModel", m_alarmModel));
    m_stack->addWidget(createQmlTab("qrc:/acsettingUI.qml", "zoneModel", m_zoneModel));

    mainLayout->addWidget(m_navList);
    mainLayout->addWidget(m_stack, 1);
    mainVLayout->addWidget(contentWidget);

    connect(m_navList, &QListWidget::currentRowChanged, m_stack, &QStackedWidget::setCurrentIndex);

    QTimer *timeTimer = new QTimer(this);
    connect(timeTimer, &QTimer::timeout, this, [=](){
        m_timeLabel->setText(QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss"));
    });
    timeTimer->start(1000);
    m_timeLabel->setText(QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss"));

    QString qss = R"(
        QMainWindow { background-color: #0F172A; }
        QListWidget { background-color: #1E293B; border: none; outline: none; color: #94A3B8; font-size: 16px; font-family: "Microsoft YaHei"; padding-top: 20px; }
        QListWidget::item { height: 60px; padding-left: 30px; border-left: 4px solid transparent; }
        QListWidget::item::selected { background-color: #0F172A; color: #38BDF8; border-left: 4px solid #38BDF8; font-weight: bold; }
        QListWidget::item:hover { background-color: #334155; color: #E2E8F0; }
    )";
    setStyleSheet(qss);
}

QStringList MainWindow::getRecentDates() {
    QStringList dates;
    QDate current = QDate::currentDate();
    for (int i = 0; i < 7; ++i) {
        dates.append(current.addDays(-i).toString("yyyy-MM-dd"));
    }
    return dates;
}