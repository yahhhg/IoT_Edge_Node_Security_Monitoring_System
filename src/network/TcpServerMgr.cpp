#include "TcpServerMgr.h"

TcpServerMgr::TcpServerMgr(QObject *parent)
	: QObject(parent)
{
    //在构造函数中初始化服务器
	m_tcpServer = new QTcpServer(this);

    //初始化数据库
    DatabaseManager::getInstance().initDatabase();

}

TcpServerMgr::~TcpServerMgr()
{
    // 先停止客户端线程，再关闭服务器（避免线程持有socket导致崩溃）
    for (auto it = m_threadList.begin(); it != m_threadList.end(); ) {
        QThread* m_clientThread = *it;
        if (m_clientThread) {
            m_clientThread->quit();
            // 给wait()加3秒超时，超时则强制终止线程
            if (!m_clientThread->wait(3000)) {
                m_clientThread->terminate();
                m_clientThread->wait();
            }
            delete m_clientThread;
            m_clientThread = nullptr;
        }
        // 无论指针是否有效，都从列表移除，并更新迭代器
        it = m_threadList.erase(it);

    }

    //清空客户端列表和线程列表
    m_clientList.clear();
    m_threadList.clear();
    
    //写入数据库
    DatabaseManager::getInstance().insertAuditLog("设备上线","服务器关闭");

    // 关闭服务器，停止接受新连接
    if (m_tcpServer) {
        m_tcpServer->close();
        m_tcpServer->deleteLater();
    }
}

//启动监听 输入端口号 返回是否成功
bool TcpServerMgr::startListen(int port)
{
    bool isSuccess=m_tcpServer->listen(QHostAddress::Any, port);
    if (isSuccess) 
    {
        //连接新连接信号 设置为不重复
        connect(m_tcpServer, &QTcpServer::newConnection, this, &TcpServerMgr::onNewConnection,Qt::UniqueConnection);
    }
    else 
    {
        qDebug() << "监听失败！"<<Qt::endl;      
    }
    return isSuccess;
}

//关闭监听
void TcpServerMgr::closeListen()
{
    // 先停止客户端线程，再关闭服务器（避免线程持有socket导致崩溃）
    for (auto it = m_threadList.begin(); it != m_threadList.end(); ) {
        QThread* m_clientThread = *it;
        if (m_clientThread) {
            m_clientThread->quit();
            // 给wait()加3秒超时，超时则强制终止线程
            if (!m_clientThread->wait(3000)) {
                m_clientThread->terminate();
                m_clientThread->wait();
            }
            delete m_clientThread;
            m_clientThread = nullptr;
        }
        // 无论指针是否有效，都从列表移除，并更新迭代器
        it = m_threadList.erase(it);

    }

    //清空客户端列表和线程列表
    m_clientList.clear();
    m_threadList.clear();

    //更新UI
    emit updateListUI("", -1);

    //关闭服务器，停止接受新连接
    m_tcpServer->close();
}

//处理新连接槽
void TcpServerMgr::onNewConnection()
{
    //获取连接队列中下一个连接
    QTcpSocket* socket = m_tcpServer->nextPendingConnection();

    //创建线程和客户端处理对象并将其移动到新线程中
    QThread* thread = new QThread(this);
    TcpClientHandler* handler = new TcpClientHandler(socket);          //传入 socket 对象来实例化

    //添加到类列表中
    m_clientList.append(handler);                                      //客户端处理对象添加到客户端列表中
    m_threadList.append(thread);                                       //将线程对象添加到线程列表中

    //将客户端处理类移动到线程中
    handler->moveToThread(thread);

    // 线程结束自动清理客户端处理对象
    connect(thread, &QThread::finished, handler, &QObject::deleteLater);

    //连接信号与槽
    //客户端断开连接时，线程结束 仅连接一次，防止重复释放
    connect(handler, &TcpClientHandler::clientDisconnected, this, &TcpServerMgr::onClientDisconnected, Qt::UniqueConnection);
    //客户端返回打印信息时
    connect(handler,&TcpClientHandler::logReceived,this,&TcpServerMgr::onClientPrint, Qt::UniqueConnection);
    //传感器值更新 转发给UI
    connect(handler, &TcpClientHandler::updateSensorData, this, &TcpServerMgr::updateSensorValue, Qt::UniqueConnection);

    //线程开始运行
    thread->start();

    //拼接连接设备的名称,当前设备的编号
    QString deviceName = socket->peerName() + " " + socket->peerAddress().toString() + ":" + QString::number(socket->peerPort());

    //写入数据库 类型：设备上线
    QString str=deviceName+"上线";
    DatabaseManager::getInstance().insertAuditLog("设备上线", str);

    //包装信号发送
    emit updateListUI(deviceName, m_clientList.count());

}

//处理客户端断开连接 将处理客户端对象的线程和线程从列表中移除
void TcpServerMgr::onClientDisconnected()
{
    //判断指针是否存在，若直接停止监听，则指针为空 可跳过移除逻辑
    if (sender())
    {
        //获取发送信号的对象所在的线程
        QThread* m_clientThread = qobject_cast<QThread*>(sender()->thread());

        //获取当前选中的设备
        TcpClientHandler* m_client = qobject_cast<TcpClientHandler*>(sender());

        //获取设备名称 地址端口
        QString deviceName = m_client->getHostName() + " " + m_client->getIpAddress() + ":" + QString::number(m_client->getPort());

        //从列表中移除
        m_threadList.removeOne(m_clientThread);                               //从线程列表中移除
        m_clientList.removeOne(qobject_cast<TcpClientHandler*>(sender()));    //从客户端列表中移除

        /* 停止客户端线程 */
        //先停止客户端线程
        m_clientThread->quit();
        // 给wait()加3秒超时，超时则强制终止线程
        if (!m_clientThread->wait(3000)) {
            //强制清理线程
            m_clientThread->terminate();
            m_clientThread->wait();
        }
        delete m_clientThread;              //立刻删除
        m_clientThread = nullptr;

        //写入数据库 类型：设备上线
        QString str=deviceName+"下线";
        DatabaseManager::getInstance().insertAuditLog("设备下线", str);

        //更新UI
        emit updateListUI("", m_clientList.indexOf(qobject_cast<TcpClientHandler*>(sender())));
    }
}

//处理UI发送指令操作
void TcpServerMgr::onCmdFromUI(QString& selectedCommand, QString& param)
{ 
    if (m_clientList.isEmpty())
        return;
    //获取当前选中的设备
    TcpClientHandler* m_client = m_clientList.at(m_curClientIndex);

    //获取执行命令
    m_lastCmd= selectedCommand + " " + param;


    // 把下拉框文本映射成约定的cmd指令名
    QString cmd;
    if (selectedCommand == "修改温度阈值") {
        cmd = "set_temp_threshold";
    }
    else if (selectedCommand == "修改湿度阈值") {
        cmd = "set_hum_threshold";
    }
    else if (selectedCommand == "重启采集程序") {
        cmd = "restart_collect";
        param = "";                                             // 重启指令不需要参数，强制清空
    }

    //获取执行命令
    QString command = cmd + " " + param;

    if (m_client) {
        //发送指令
        m_client->sendCommand(cmd, param);
    }
}

//处理选中设备发生变化
void TcpServerMgr::onSelectedDeviceChanged(int index)
{
    //更新设备编号
    m_curClientIndex = index;

    //若当前什么也没选中
    if (index == -1 || m_clientList.isEmpty())
    {
        return;
    }

    //获取当前选中的设备
    TcpClientHandler* m_client = m_clientList.at(m_curClientIndex);

    if (m_client)
    {
        //获取传感器数据
        SensorData data = m_client->getSensorData();

        //发送信号
        emit updateSensorValueToUI(data);
    }
}

//处理历史数据查询槽
void TcpServerMgr::onQueryHistoryData(const QString& startTime, const QString& endTime)
{
    //获取SQL查询结果
    QList<Sql_SensorData> result= DatabaseManager::getInstance().querySensorDataByTimeRange(startTime, endTime);
    
    //封装为信号发送回UI
    emit sendHistoryDataToUI(result);
}

//处理安全日志审计槽
void TcpServerMgr::onQuerySecurityLog(const QString& type)
{
    //获取SQL查询结果
    QList<Sql_AuditLog> result = DatabaseManager::getInstance().queryAuditLogByType(type);

    //封装为信号发送回UI
    emit sendSecurityLogToUI(result);
}

//处理客户端返回的树莓派需打印信息
void TcpServerMgr::onClientPrint(QString msg,int flag)
{
    //获取当前选中的设备
    TcpClientHandler* m_client = qobject_cast<TcpClientHandler*>(sender());

    //获取设备名称 地址端口
    QString deviceName = m_client->getHostName() + " " + m_client->getIpAddress() + ":" + QString::number(m_client->getPort());
    //获取当前时间
    QString time=QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss");

    //拼接信息
    QString printMsg = time + " " + deviceName + "\n" + msg;

    //写入数据库
    if (flag<2)
    {
        bool isSuccess= msg.contains("成功");
        DatabaseManager::getInstance().insertAuditLog("下发指令", m_lastCmd, isSuccess);
    }
   
      

    //发送信号
    emit printToUI(printMsg);
}

//处理客户端解析的传感器更新值
void TcpServerMgr::updateSensorValue()
{
    //获取当前选中的设备
    TcpClientHandler* m_client = qobject_cast<TcpClientHandler*>(sender());

    //获取设备信息
    int index = m_clientList.indexOf(m_client);        //编号
    QString deviceName = m_client->getHostName() + " " + m_client->getIpAddress() + ":" + QString::number(m_client->getPort()); //设备名称

    //获取传感器数据
    SensorData data = m_client->getSensorData();

    //写入到数据库历史信息中
    DatabaseManager::getInstance().insertSensorData(deviceName,data.temperature,data.humidity,data.gas_concentration);

    //若发送更新信号者不为当前设备,直接忽略发送信号更新UI
    if (index != m_curClientIndex) 
    {
        return;
    }

    //发送信号
    emit updateSensorValueToUI(data);

    qDebug() <<"update" << data.temperature;
}