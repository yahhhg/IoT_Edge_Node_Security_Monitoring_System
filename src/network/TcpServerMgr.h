#ifndef TCPSERVERMAR_H
#define TCPSERVERMAR_H

#if 0
   此类为TCP服务器控制类，其维护一个 TcoServer 对象，并对外提供实现监听，多客户端管理的接口
   其实现了服务器的基本功能，并且管理多客户端，将他们分别移动到一个线程中
#endif

#include <QObject>
#include <QtNetwork/QTcpServer>            //TCP服务器类
#include <QThread>                         //线程类
#include <QStandardItemModel>              //标准模型类,用于维护历史数据查询和安全日志审计模型

#include "src/network/TcpClientHandler.h"  //客户端处理类
#include "src/core/DatabaseManager.h"    //数据库管理类

class TcpServerMgr  : public QObject
{
	Q_OBJECT
    QTcpServer *m_tcpServer;
	
	

	//连接的客户端列表
    QList<TcpClientHandler*> m_clientList;
	//客户端线程列表
	QList<QThread*> m_threadList;

	//当前操作的客户端编号 即为连接客户端的下标
	int m_curClientIndex=0;
	//记录上次执行的指令
    QString m_lastCmd="";

public:

	TcpServerMgr(QObject *parent);
	~TcpServerMgr();

	/* 提供的外部接口 */
	//启动监听 
	bool startListen(int port);
	//关闭监听
    void closeListen();

signals:
	/* 发送UI */
	//更新UI 使设备管理列表更新 
	void updateListUI(QString,int);
	//发送需打印信息到UI
    void printToUI(QString);
	//发送传感器更新值到UI
    void updateSensorValueToUI(const SensorData&);
	//发送历史数据查询值到UI
    void sendHistoryDataToUI(QList<Sql_SensorData>&);
    //发送安全日志审计值到UI
    void sendSecurityLogToUI(QList<Sql_AuditLog>&);


public slots:         //面向主窗口 绑定在主窗口文件中
	/* 来自UI */
	//处理UI发送指令操作槽
	void onCmdFromUI(QString& cmd, QString& param);
	//处理选中设备发生变化槽
	void onSelectedDeviceChanged(int index);
	//处理历史数据查询槽
    void onQueryHistoryData(const QString& startTime, const QString& endTime);
	//处理安全日志审计查询槽
    void onQuerySecurityLog(const QString& type);

private slots:
	/* 来自本地 */
	//处理新连接槽
	void onNewConnection();

	/* 来自客户端 */
	//处理客户端断开连接
	void onClientDisconnected();
	//处理客户端返回的树莓派需打印信息
    void onClientPrint(QString msg,int flag);
	//处理客户端解析的传感器更新值
	void updateSensorValue();

	

};

#endif // TCPSERVERMAR_H