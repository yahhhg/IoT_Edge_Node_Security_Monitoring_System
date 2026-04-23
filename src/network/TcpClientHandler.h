#ifndef TCPCLIENTHANDLER_H
#define TCPCLIENTHANDLER_H

#if 0
此类为服务器端处理类：负责在独立线程中处理与单个树莓派客户端的所有网络通信操作

约定的JSON数据包格式：
{
   "type": "数据类型",    // 固定类型，有正常指令，心跳包类型
   "cmd" : "指令类型",     // 固定指令名，对应下拉框的3个选项
   "param" : "参数内容",  // 改阈值时填数字，重启指令留空
}
发送单元格式: 长度前缀法
4字节长度前缀 + JSON数据
#endif

#include <QObject>
#include <QtNetwork/QTcpSocket>           //TCP连接 Socket 类                    
#include <QTimer>                         //定时器类


//传感器数据结构体
struct SensorData
{
    double temperature;         // 温度
    double humidity;            // 湿度
    double gas_concentration;   // 气体浓度
    int is_alarm;               // 报警状态，使用类似Linux文件权限的数字表示法 4代表温度报警，2代表湿度报警，1代表气体浓度报警，最后取和 0代表无报警
};


// TcpClientHandler类：负责在独立线程中处理与单个树莓派客户端的所有网络通信
class TcpClientHandler : public QObject
{
    Q_OBJECT
     
    // 与下位机通信的TCP套接字     
    QTcpSocket* m_socket; 
    //数据接收缓冲区
    QByteArray m_buffer;
    //当前传感器数据
    SensorData m_sensorData;

    // 发送带长度前缀的JSON数据 长度前缀法的核心
    void sendData(const QJsonObject& obj);
    // 处理接收缓冲区中的数据，解析协议
    void processData();
    //解析传感器数据并发送信号
    void parseSensorData(const QJsonObject& obj);


public:
    // 构造函数：接收已连接的QTcpSocket对象，parent为父对象
    explicit TcpClientHandler(QTcpSocket* socket, QObject* parent = nullptr);
    ~TcpClientHandler();

    //返回IP地址
    QString getIpAddress() const { return m_socket->peerAddress().toString(); }
    //返回端口号
    quint16 getPort() const { return m_socket->peerPort(); }
    //返回主机名称
    QString getHostName() const { return m_socket->peerName(); }
    //返回当前传感器数据
    SensorData getSensorData() const { return m_sensorData; }

signals:
    // 当客户端断开连接时发出信号
    void clientDisconnected();
    // 树莓派的返回结果 内容+类型   0指令执行失败 1指令执行成功  2其他
    void logReceived(QString,int flag=2);
    //更新传感器的UI，传递参数为传感器数据
    void updateSensorData();

public slots:
    /* 来自服务器 */
    //发送指令
    void sendCommand(const QString& command, const QString& param);
    
private slots:
    /* 来自本地 */
    // 当有数据可读时调用
    void onReadyRead();
    // 当连接关闭时调用
    void onDisconnected();
    
    /* 来自客户端 */
   

};
#endif // TCPCLIENTHANDLER_H