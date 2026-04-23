#ifndef DATABASEMANAGER_H
#define DATABASEMANAGER_H

#if 0
    此类为数据库管理类，负责数据库的连接，写入查询，多表管理
    维护历史数据表和安全审计表,其中存放对于所有设备的记录
    使用 SQLite 嵌入式数据库（文件型数据库，一个文件就是一个数据库）
    使用单例模式，静态返回一个实例，确保全局只有一个实例，其他类也共享一个实例而不用实例化对象使用
#endif

#include <QObject>
//数据库
#include <QtSql/QSqlDatabase>         //数据库类
#include <QtSql/QSqlQuery>            //数据库操作类 用于执行 SQL 语句（增删改查）
#include <QtSql/QSqlError>            //数据库错误类

// 传感器数据结构体，用于历史数据查询结果封装
struct Sql_SensorData {
        QString timestamp;              //时间
        QString ip_port;                //设备IP和端口
        double temperature;             //温度
        double humidity;                //湿度
        double gas_concentration;       //气体浓度
    };

// 安全审计日志结构体，用于审计查询结果封装
struct Sql_AuditLog {
        QString operationTime;          //操作时间
        QString operationType;          //操作类型  区分查询的根据
        QString operationContent;       //操作内容
        QString operationResult;        //操作结果
};

class DatabaseManager  : public QObject
{
	Q_OBJECT

    // 数据库连接对象
    QSqlDatabase m_db;

public:
	DatabaseManager(QObject *parent=nullptr);
	~DatabaseManager();

    /* 对外接口 */
    // 静态接口：获取单例实例
    static DatabaseManager& getInstance() {
        static DatabaseManager instance;
        return instance;
    }
    
    // 初始化数据库，创建连接和表
    bool initDatabase();

    // 插入一条传感器历史数据
    bool insertSensorData(QString ip_port, double temp, double hum, double gas);
    // 按时间范围查询传感器历史数据
    QList<Sql_SensorData> querySensorDataByTimeRange(const QString& startTime, const QString& endTime);

    // 插入一条安全审计日志
    bool insertAuditLog(const QString& operationType, const QString& content, bool isSuccess = true);
    // 按操作类型查询安全审计日志，传入"全部"则查询所有
    QList<Sql_AuditLog> queryAuditLogByType(const QString& operationType);
};

#endif // DATABASEMANAGER_H