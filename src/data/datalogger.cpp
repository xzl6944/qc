#include "datalogger.h"
#include <QSqlQuery>       // SQL执行查询、语句预处理
#include <QSqlError>       // SQL错误信息类(本代码未实际使用)
#include <QDebug>          // qDebug打印调试输出
#include <QDateTime>       // Qt日期时间工具类
#include <QVariantList>    // Qt数组容器，可传递给QML

// 构造函数，初始化父QObject，数据库成员m_db默认空
DataLogger::DataLogger(QObject *parent) : QObject(parent)
{

}

/**
 * @brief 初始化数据库，打开sqlite，创建所有业务表，初始化默认分区配置
 * @return true打开/初始化成功；false打开数据库失败
 */
bool DataLogger::initDatabase()
{
    // 判断是否已经存在命名数据库连接
    if (QSqlDatabase::contains("qt_sql_default_connection"))
    {
        // 复用已有的数据库连接
        m_db = QSqlDatabase::database("qt_sql_default_connection");
    }
    else
    {
        // 创建SQLite驱动的命名连接
        m_db = QSqlDatabase::addDatabase("QSQLITE", "qt_sql_default_connection");
        // 设置sqlite数据库文件名，程序运行同目录生成 ac_energy_data.db
        m_db.setDatabaseName("ac_energy_data.db");
    }

    // 数据库打开失败直接返回false
    if (!m_db.open())
        return false;

    QSqlQuery query(m_db); // 创建查询对象，绑定当前数据库连接

    // 创建实时分区数据表，如果不存在；存储每个分区采样的实时状态
    query.exec("CREATE TABLE IF NOT EXISTS realtime_log ("
               "id INTEGER PRIMARY KEY AUTOINCREMENT, " //自增主键
               "timestamp DATETIME, "                   //记录时间
               "zone_id INTEGER, "                      //分区ID
               "current_temp REAL, "                    //当前温度
               "target_temp REAL, "                     //目标温度
               "valve_opening REAL, "                   //阀门开度百分比
               "co2_level INTEGER, "                    //CO2浓度
               "power_kw REAL)");                       //分区功率kW

    // 创建总能耗记录表 total_energy_log，新增total_power总功率字段
    query.exec("CREATE TABLE IF NOT EXISTS total_energy_log ("
               "id INTEGER PRIMARY KEY AUTOINCREMENT, "
               "timestamp DATETIME, "
               "total_kwh REAL, "      //总耗电量 kWh
               "saved_kwh REAL, "      //节约电量 kWh
               "co2_reduction REAL, "  //CO2减排 kg
               "total_power REAL)");   //系统总功率 kW

    // 兼容旧版本数据库：尝试新增total_power列；列已存在时ALTER语句会静默失败，属于预期
    QSqlQuery alterQuery(m_db);
    alterQuery.exec("ALTER TABLE total_energy_log ADD COLUMN total_power REAL");

    // 创建告警日志表
    query.exec("CREATE TABLE IF NOT EXISTS alarm_log ("
               "id INTEGER PRIMARY KEY AUTOINCREMENT, "
               "timestamp DATETIME, "
               "level INTEGER, "    //告警等级
               "message TEXT)");    //告警描述文本

    // 设备配置表：存储4个分区持久化配置
    query.exec("CREATE TABLE IF NOT EXISTS device_config ("
               "zone_id INTEGER PRIMARY KEY, " //分区ID主键
               "zone_name TEXT, "             //分区名称
               "power_state INTEGER, "        //电源状态 0关1开
               "ac_mode INTEGER, "            //空调模式
               "user_set_temp REAL)");        //用户设定温度

    // 查询配置表里一共有多少条记录
    query.exec("SELECT COUNT(*) FROM device_config");

    // 如果表为空，插入4个分区的默认配置
    if (query.next() && query.value(0).toInt() == 0)
    {
        for(int i = 1; i <= 4; ++i)
        {
            //预处理插入语句，使用bind绑定参数，防止SQL注入
            query.prepare("INSERT INTO device_config "
                          "(zone_id, zone_name, power_state, ac_mode, user_set_temp) "
                          "VALUES (?, ?, ?, ?, ?)");
            query.addBindValue(i); //分区ID

            //根据id赋值分区名字
            query.addBindValue((i==1)?"开放办公区": ((i==2)?"会议室":((i==3)?"经理办公室":"走廊/大堂")));
            query.addBindValue(1);    //默认上电打开
            query.addBindValue(0);    //默认模式0制冷
            query.addBindValue(24.0); //默认设定温度24℃
            query.exec(); //执行插入
        }
    }

    return true; //数据库初始化完成返回true
}

/**
 * @brief 批量记录各个分区实时运行数据
 * @param zones 分区数据列表
 */
void DataLogger::logRealtimeZones(const QList<ZoneData> &zones)
{
    QSqlQuery query(m_db);
    m_db.transaction(); //开启数据库事务，批量插入提升性能

    //预处理SQL语句
    query.prepare("INSERT INTO realtime_log "
                  "(timestamp, zone_id, current_temp, target_temp, valve_opening, co2_level, power_kw) "
                  "VALUES (?, ?, ?, ?, ?, ?, ?)");

    //获取当前系统时间，格式化字符串
    QString now = QDateTime::currentDateTime().toString("yyyy-MM-dd HH:mm:ss");

    //遍历所有分区，循环绑定参数执行
    for (const auto &z : zones)
    {
        query.addBindValue(now);
        query.addBindValue(z.zoneId);
        query.addBindValue(z.currentTemp);
        query.addBindValue(z.setTemp);
        query.addBindValue(z.valveOpening);
        query.addBindValue(z.co2Level);

        //功率计算公式：开机则基础0.1 + 阀门开度比例*1.25；关机功率0
        float power = z.isPowerOn ? (0.1f + (z.valveOpening / 100.0f) * 1.25f) : 0.0f;
        query.addBindValue(power);

        query.exec(); //执行单条插入
    }

    m_db.commit(); //提交事务，批量写入数据库
}

/**
 * @brief 写入系统总能耗记录
 * @param totalKwh 总耗电量
 * @param savedKwh 节约电量
 * @param co2Red CO2减排量
 * @param totalPower 系统总功率
 */
void DataLogger::logTotalEnergy(float totalKwh, float savedKwh, float co2Red, float totalPower)
{
    QSqlQuery query(m_db);
    //预处理插入SQL
    query.prepare("INSERT INTO total_energy_log "
                  "(timestamp, total_kwh, saved_kwh, co2_reduction, total_power) "
                  "VALUES (?, ?, ?, ?, ?)");

    //绑定当前时间
    query.addBindValue(QDateTime::currentDateTime().toString("yyyy-MM-dd HH:mm:ss"));
    query.addBindValue(totalKwh);
    query.addBindValue(savedKwh);
    query.addBindValue(co2Red);
    query.addBindValue(totalPower);

    query.exec(); //执行插入
}

/**
 * @brief 分页查询某一天总能耗历史，供QML调用
 * @param dateStr 日期 yyyy‑MM‑dd
 * @param page 当前页码，从1开始
 * @param pageSize 每页记录数量
 * @return QVariantMap {dataList:[], totalPages:int, totalCount:int}
 */
QVariantMap DataLogger::getTotalEnergyHistory(const QString &dateStr, int page, int pageSize)
{
    QVariantMap result;  //返回结果外层map
    QVariantList list;   //存放本页数据数组
    QSqlQuery query(m_db);

    //第一步：统计该日期条件下总记录条数
    query.prepare("SELECT COUNT(*) FROM total_energy_log WHERE timestamp LIKE ?");
    query.addBindValue(dateStr + "%"); //like模糊匹配，匹配当天所有时间 "2026‑10‑28%"
    query.exec();

    int totalCount = 0;
    if(query.next())
        totalCount = query.value(0).toInt();

    //计算总页数：向上取整 (总数+页大小‑1)/页大小
    int totalPages = (totalCount + pageSize - 1) / pageSize;
    if(totalPages == 0)
        totalPages = 1; //没有数据也至少1页

    //计算偏移量，page从1开始 offset=(page‑1)*pageSize
    int offset = (page - 1) * pageSize;

    //第二步：分页查询数据，按id倒序（新记录在前） LIMIT每页条数 OFFSET偏移
    query.prepare("SELECT timestamp, total_kwh, saved_kwh, co2_reduction, total_power "
                  "FROM total_energy_log WHERE timestamp LIKE ? "
                  "ORDER BY id DESC LIMIT ? OFFSET ?");
    query.addBindValue(dateStr + "%");
    query.addBindValue(pageSize);
    query.addBindValue(offset);
    query.exec();

    //循环读取查询结果，封装成QVariantMap放入list
    while (query.next())
    {
        QVariantMap map;
        map["time"] = query.value(0).toString();                           //时间字符串
        map["total_kwh"] = QString::number(query.value(1).toFloat(), 'f', 3) + " kWh"; //总能耗，保留3位小数带单位
        map["saved_kwh"] = QString::number(query.value(2).toFloat(), 'f', 3) + " kWh"; //节约电量
        map["co2"] = QString::number(query.value(3).toFloat(), 'f', 3) + " kg";        //CO2减排
        map["power"] = QString::number(query.value(4).toFloat(), 'f', 2) + " kW";       //总功率保留2位小数

        list.append(map); //加入结果数组
    }

    result["dataList"] = list;      //当前页数据数组
    result["totalPages"] = totalPages; //总页数
    result["totalCount"] = totalCount; //符合条件总记录数

    return result;
}

/**
 * @brief 记录告警日志
 * @param level 告警等级
 * @param message 告警信息文本
 */
void DataLogger::logAlarm(int level, const QString &message)
{
    QSqlQuery query(m_db);
    query.prepare("INSERT INTO alarm_log (timestamp, level, message) VALUES (?, ?, ?)");
    query.addBindValue(QDateTime::currentDateTime().toString("yyyy-MM-dd HH:mm:ss"));
    query.addBindValue(level);
    query.addBindValue(message);
    query.exec();
}

/**
 * @brief 更新保存各个分区设备配置，批量update
 * @param zones 分区数据列表
 */
void DataLogger::saveDeviceConfig(const QList<ZoneData> &zones)
{
    QSqlQuery query(m_db);
    m_db.transaction(); //开启事务加速批量更新

    //预处理更新语句，按zone_id更新对应分区配置
    query.prepare("UPDATE device_config SET power_state=?, ac_mode=?, user_set_temp=? WHERE zone_id=?");
    for (const auto &z : zones)
    {
        query.addBindValue(z.isPowerOn ? 1 : 0);          //电源状态转int
        query.addBindValue(z.mode == ACMode::Cooling ? 0 : 1); //模式：制冷0，其他1
        query.addBindValue(z.userSetTemp);
        query.addBindValue(z.zoneId);
        query.exec();
    }
    m_db.commit(); //提交事务
}

/**
 * @brief 加载设备配置；修改后直接返回空列表
 * @return 空QList，上层不再读取数据库存储的配置，使用初始化预置默认值
 */
QList<ZoneData> DataLogger::loadDeviceConfig()
{
    //废弃读取数据库配置逻辑，直接返回空列表
    //程序启动使用initDatabase初始化插入到db里的默认配置
    return QList<ZoneData>();
}
