#include "DatabaseManager.h"

DatabaseManager::DatabaseManager(QObject *parent)
	: QObject(parent)
{}

DatabaseManager::~DatabaseManager()
{
    // 自动关闭数据库连接
    if (m_db.isOpen()) {
        m_db.close();
    }
}

// 初始化数据库
bool DatabaseManager::initDatabase()
{
    /* 初始化数据库连接 */
    // 检查是否已经连接过，避免重复连接
    if (QSqlDatabase::contains("qt_SensorDatabase_connection")) {
        m_db = QSqlDatabase::database("qt_SensorDatabase_connection");        //连接名称
    }
    else {
        // 创建数据库连接："QSQLITE" 是 SQLite 驱动名，Qt 内置
        m_db = QSqlDatabase::addDatabase("QSQLITE");
        // 设置数据库文件名（SQLite 是文件型数据库，这里会在程序运行目录下生成一个 .db 文件）
        m_db.setDatabaseName("QSQL_sensor.db");
    }

    // 打开数据库连接
    if (!m_db.open()) {
        // 打开失败，打印错误信息
        qDebug() << "数据库打开失败：" << m_db.lastError().text();
        return false;
    }
    qDebug() << "数据库打开成功！";

    /* 创建表 */
    // 创建QSqlQuery对象用于执行SQL语句
    QSqlQuery query;

    // 准备创建传感器历史数据表的SQL语句，使用IF NOT EXISTS避免重复创建报错
    // 自增主键id、IP地址和端口，时间戳、温度、湿度、气体浓度六个字段
    QString createSensorTableSql = R"(
        CREATE TABLE IF NOT EXISTS sensor_data (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            timestamp TEXT NOT NULL,
            ip_port TEXT NOT NULL,
            temperature REAL NOT NULL,
            humidity REAL NOT NULL,
            gas_concentration REAL NOT NULL
        )
    )";

    // 执行建表SQL，如果失败则打印错误信息并返回false
    if (!query.exec(createSensorTableSql)) {
        qDebug() << "创建传感器数据表失败，错误信息：" << query.lastError().text();
        return false;
    }

    // 准备创建安全审计日志表的SQL语句，同样使用IF NOT EXISTS
    // 自增主键id、操作时间、操作类型（区分依据）、操作内容、操作结果、五个字段
    QString createAuditTableSql = R"(
        CREATE TABLE IF NOT EXISTS operation_audit_log (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            operation_time TEXT NOT NULL,
            operation_type TEXT NOT NULL,
            operation_content TEXT NOT NULL,
            operation_result TEXT NOT NULL
        )
    )";

    // 执行审计日志表的建表SQL，如果失败则打印错误信息并返回false
    if (!query.exec(createAuditTableSql)) {
        qDebug() << "创建安全审计日志表失败，错误信息：" << query.lastError().text();
        return false;
    }

    // 所有表创建成功，返回true
    qDebug() << "数据库表初始化完成";
    return true;
}

// 插入一条传感器历史数据
bool DatabaseManager::insertSensorData(QString ip_port,double temp, double hum, double gas)
{
    // 准备插入传感器数据的SQL语句，使用?作为占位符，可以防止SQL注入并自动处理字符串转义
    QString insertSql = "INSERT INTO sensor_data (timestamp, ip_port, temperature, humidity, gas_concentration) "
        "VALUES (?, ?, ?, ?, ?)";

    QSqlQuery query;
    // 先准备SQL语句，如果准备失败则打印错误信息并返回false
    if (!query.prepare(insertSql)) {
        qDebug() << "准备插入传感器数据SQL失败，错误信息：" << query.lastError().text();
        return false;
    }

    // 按顺序给占位符绑定具体的数值，第一个?绑定当前时间，格式统一为yyyy-MM-dd HH:mm:ss
    query.addBindValue(QDateTime::currentDateTime().toString("yyyy-MM-dd HH:mm:ss"));
    query.addBindValue(ip_port);
    query.addBindValue(temp);
    query.addBindValue(hum);
    query.addBindValue(gas);

    // 执行插入操作，如果失败则打印错误信息并返回false
    if (!query.exec()) {
        qDebug() << "插入传感器数据失败，错误信息：" << query.lastError().text();
        return false;
    }

    // 插入成功，返回true
    return true;
}

// 按时间范围查询传感器历史数据
QList<Sql_SensorData> DatabaseManager::querySensorDataByTimeRange(const QString& startTime, const QString& endTime)
{
    // 存储查询结果
    QList<Sql_SensorData> resultList;

    // 准备按时间范围查询的SQL语句，使用BETWEEN筛选起止时间，ORDER BY按时间正序排列
    QString querySql = "SELECT timestamp, ip_port, temperature, humidity, gas_concentration "
        "FROM sensor_data "
        "WHERE timestamp BETWEEN ? AND ? "
        "ORDER BY timestamp ASC";

    QSqlQuery query;
    // 准备SQL语句
    if (!query.prepare(querySql)) {
        qDebug() << "准备查询传感器数据SQL失败，错误信息：" << query.lastError().text();
        return resultList;
    }

    // 绑定起止时间参数 绑定到两个 ？上
    query.addBindValue(startTime);
    query.addBindValue(endTime);

    // 执行查询，如果成功则遍历结果
    if (query.exec()) {
        // 循环移动到下一条数据，直到没有数据返回false
        while (query.next()) {
            // 创建一个SensorData结构体对象，用于存储单条数据
            Sql_SensorData data;
            // 通过字段名从查询结果中获取数值，并赋值给结构体成员
            data.timestamp = query.value("timestamp").toString();
            data.ip_port = query.value("ip_port").toString();
            data.temperature = query.value("temperature").toDouble();
            data.humidity = query.value("humidity").toDouble();
            data.gas_concentration = query.value("gas_concentration").toDouble();
            // 将填充好的结构体添加到结果列表中
            resultList.append(data);
        }
    }
    else {
        // 查询失败，打印错误信息
        qDebug() << "查询传感器数据失败，错误信息：" << query.lastError().text();
    }

    return resultList;
}

// 插入一条安全审计日志
bool DatabaseManager::insertAuditLog(const QString& operationType, const QString& content, bool isSuccess)
{
    // 准备插入审计日志的SQL语句，同样使用?占位符 更安全
    QString insertSql = "INSERT INTO operation_audit_log (operation_time, operation_type, operation_content, operation_result) "
        "VALUES (?, ?, ?, ?)";

    //准备待执行SQL语句
    QSqlQuery query;
    if (!query.prepare(insertSql)) {
        qDebug() << "准备插入审计日志SQL失败，错误信息：" << query.lastError().text();
        return false;
    }

    // 绑定参数，操作时间自动取当前时间，操作结果根据isSuccess参数转换为"成功"或"失败"
    query.addBindValue(QDateTime::currentDateTime().toString("yyyy-MM-dd HH:mm:ss"));
    query.addBindValue(operationType);
    query.addBindValue(content);
    query.addBindValue(isSuccess ? "成功" : "失败");

    //执行SQL语句
    if (!query.exec()) {
        qDebug() << "插入审计日志失败，错误信息：" << query.lastError().text();
        return false;
    }

    return true;
}

// 按操作类型查询安全审计日志，传入"全部"则查询所有
QList<Sql_AuditLog> DatabaseManager::queryAuditLogByType(const QString& operationType)
{
    // 存储查询结果
    QList<Sql_AuditLog> resultList;

    // 准备基础SQL语句
    QString querySql = "SELECT operation_time, operation_type, operation_content, operation_result "
        "FROM operation_audit_log ";

    // 如果传入的操作类型不是"全部"，则添加WHERE条件筛选特定类型
    if (operationType != "全部") {
        querySql += "WHERE operation_type = ? ";
    }

    // 添加ORDER BY子句，按操作时间倒序排列，最新的操作显示在最前面
    querySql += "ORDER BY operation_time DESC";

    //准备待执行SQL语句
    QSqlQuery query;
    if (!query.prepare(querySql)) {
        qDebug() << "准备查询审计日志SQL失败，错误信息：" << query.lastError().text();
        return resultList;
    }

    // 如果不是查询全部，则绑定操作类型参数
    if (operationType != "全部") {
        query.addBindValue(operationType);
    }

    //执行SQL语句
    if (query.exec()) {
        while (query.next()) {
            Sql_AuditLog log;
            log.operationTime = query.value("operation_time").toString();
            log.operationType = query.value("operation_type").toString();
            log.operationContent = query.value("operation_content").toString();
            log.operationResult = query.value("operation_result").toString();
            resultList.append(log);
        }
    }
    else {
        qDebug() << "查询审计日志失败，错误信息：" << query.lastError().text();
    }

    return resultList;
}