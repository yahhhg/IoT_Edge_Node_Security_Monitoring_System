#include <QJsonObject>                   //JSON 对象类，用于写入键值对 
#include <QJsonDocument>                 //JSON 文档类，用于将 JSON 对象转换为 JSON 文档，以便将其转换为字节数组进行网络传输
#include <QStandardPaths>
#include <QDir>
#include "TcpClienthandler.h"

//初始化成员变量，连接QTcpSocket的信号槽
TcpClientHandler::TcpClientHandler(QTcpSocket* socket, QObject* parent)
    : QObject(parent), m_socket(socket)
{
    // 将socket的父对象设置为当前handler，以便在handler销毁时自动关闭socket
    m_socket->setParent(this);

    //初始化传感器
    m_sensorData = { 0.0, 0.0, 0.0, false };

    // 有数据可读时
    connect(m_socket, &QTcpSocket::readyRead, this, &TcpClientHandler::onReadyRead);
    // 连接断开时
    connect(m_socket, &QTcpSocket::disconnected, this, &TcpClientHandler::onDisconnected);
}
//清理文件对象和套接字
TcpClientHandler::~TcpClientHandler()
{
    // 用abort()强制断开避免阻塞
    if (m_socket) {
        m_socket->abort();           //强制断开连接
        delete m_socket;             //删除套接字对象
    }
      // 发送信号通知服务器
    emit clientDisconnected();
}


// 发送带长度前缀的JSON数据 4字节长度前缀包+JSON数据包
void TcpClientHandler::sendData(const QJsonObject& obj)
{
    // 添加套接字状态判断，避免无效发送
    if (!m_socket || m_socket->state() != QTcpSocket::ConnectedState) {
        return;
    }
    // 将JSON对象转换为紧凑格式的JSON文档
    QJsonDocument doc(obj);
    // 将JSON文档转换为字节数组 UTF-8编码 Compact是紧凑格式，减少传输体积
    QByteArray jsonData = doc.toJson(QJsonDocument::Compact);
    // 准备4字节长度前缀
    QByteArray lengthData;
    lengthData.resize(4);
    // 将JSON数据长度转换为大端序（网络字节序） 将 Json 数据长度写入 lengthData
    // 强制转换为quint32，避免int溢出风险
    qToBigEndian<quint32>(static_cast<quint32>(jsonData.size()), reinterpret_cast<uchar*>(lengthData.data()));
    // 先发送长度前缀，再发送JSON数据
    m_socket->write(lengthData);
    m_socket->write(jsonData);
    // 刷新套接字缓冲区，确保数据立即发送
    m_socket->flush();
}

// 处理接收缓冲区中的数据，解析协议
void TcpClientHandler::processData()
{
    // 循环处理缓冲区，直到数据不足一个完整包 最小为 4 字节即一个长度前缀
    while (m_buffer.size() >= 4) {
        // 读取前4字节，转换为小端序的32位无符号整数
        quint32 length = qFromBigEndian<quint32>(reinterpret_cast<const uchar*>(m_buffer.constData()));
        // 防御性判断，避免长度为0导致的死循环
        if (length == 0) {
            m_buffer = m_buffer.mid(4);
            continue;
        }
        // 如果缓冲区数据不足（4字节长度 + 实际数据），等待更多数据
        if (m_buffer.size() < 4 + length) {
            return;
        }
        // 提取完整的JSON数据部分
        QByteArray jsonData = m_buffer.mid(4, length);
        // 从缓冲区中移除已处理的数据（4字节长度 + JSON数据）
        m_buffer = m_buffer.mid(4 + length);
        // 解析JSON数据为文档对象
        QJsonDocument doc = QJsonDocument::fromJson(jsonData);
        // 如果不是有效的JSON对象，跳过
        if (!doc.isObject()) {
            emit logReceived("警告：收到无效JSON数据包，已丢弃");
            continue;
        }
        // 获取JSON对象
        QJsonObject obj = doc.object();
        // 获取协议类型字段
        QString type = obj["type"].toString();
        // 根据协议类型分发处理
        if (type == "result" || type == "error") {
            // 命令执行结果或错误，发送logReceived信号给UI显示
            emit logReceived(obj["data"].toString(),obj["sucess"].toBool());
        }
        // 心跳包处理，收到心跳立即回应，兼容下位机心跳逻辑
        else if (type == "heartbeat") {
            sendData(QJsonObject{ {"type", "heartbeat"}, {"data", "pong"} });
        }
        //传感器检测包，收到即解析传感器数据
        else if(type== "sensor_data") {
            //解析传感器数据包并封装为信号发送
            parseSensorData(obj);
        }
        else {
            emit logReceived("警告：收到未知类型数据包，类型：" + type);
        }
    }
}

//解析传感器数据
void TcpClientHandler::parseSensorData(const QJsonObject& obj)
{
    // 解析传感器数据
    QJsonObject dataObj = obj["data"].toObject();

    m_sensorData.temperature = dataObj["temperature"].toDouble();
    m_sensorData.humidity = dataObj["humidity"].toDouble();
    m_sensorData.gas_concentration = dataObj["gas_concentration"].toDouble();
    m_sensorData.is_alarm = dataObj["is_alarm"].toInt();

    // 发送传感器数据信号
    emit updateSensorData();

    qDebug() <<"parse" << m_sensorData.temperature;
}


//发送指令
void TcpClientHandler::sendCommand(const QString& cmd, const QString& param)
{
    //拼接成约定的JSON格式
    QJsonObject jsonObj;           // 创建JSON对象
    jsonObj["type"] ="cmd";        // 填入协议类型
    jsonObj["cmd"] = cmd;          // 填入指令类型
    jsonObj["param"] = param;      // 填入参数

    //发送JSON数据
    sendData(jsonObj);
}

// 当有数据可读时调用槽
void TcpClientHandler::onReadyRead()
{
    // 将数据追加到协议解析缓冲区，然后处理
    m_buffer.append(m_socket->readAll());
    processData();
}

// 当连接关闭时调用槽
void TcpClientHandler::onDisconnected()
{
    // 强制断开Socket，彻底释放资源
    if (m_socket) {
        m_socket->abort();
    }
    // 发送信号通知服务器
    emit clientDisconnected();
}

