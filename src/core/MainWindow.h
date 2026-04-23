#pragma once

#if 0
   此类为主窗口类，负责与UI控件的交互逻辑，以及UI的更新显示
#endif

#include <QtWidgets/QMainWindow>
#include "ui_MainWindow.h"
#include "src/network/TcpServerMgr.h"            //服务器管理
#include "src/widgets/SensorChartWidget.h"       //传感器图表自定义控件


QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; };
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

    //状态栏右边的监控，设备状态
    QLabel *statusLabel;

    //服务器控制器
    TcpServerMgr *serverMgr;  

        
    //处理报警
    void do_handleAlarm(int);

private slots:
    /* UI连接槽 */
    //启动监控按钮按下槽
    void on_toggleMonitorAct_triggered(bool checked);
    //刷新设备按钮按下槽
    void on_refreshDeviceAct_triggered();
    //发送指令按钮按下槽
    void on_sendCommandBtn_clicked();
    //设备列表项选中槽
    void on_deviceListWidget_itemSelectionChanged();
    //清空编辑框按钮按下槽
    void on_clearEditBtn_clicked();
    //历史数据查询按钮按下槽
    void on_queryHistoryBtn_clicked();
    //安全审计查询按下槽
    void on_queryLogBtn_clicked();
    //一键导出历史数据菜单项
    void on_exportDataAct_triggered();
    //一键导出安全审计数据菜单项
    void on_exportLogAct_triggered();
    //退出系统菜单项
    void on_exitAct_triggered();
    //隐藏设备面板菜单项
    void on_toggleDevicePanelAct_triggered(bool checked);
    //隐藏菜单面板菜单项
    void on_toggleControlPanelAct_triggered(bool checked);
    //关于系统菜单项
    void on_aboutAct_triggered();
    //一键导出报表菜单项
    void on_exportReportAct_triggered();
   

    /* 外部响应槽 */
    //新连接时更新设备管理列表
    void do_updateListUI(QString,int);
    //打印下位机返回信息
    void do_printInfo(QString);
    //更新设备传感器状态
    void do_updateSensorState(const SensorData&);
    //更新历史数据接收
    void do_receiveQueryHistory(QList<Sql_SensorData>&);
    //更新安全审计数据接收
    void do_receiveQueryLog(QList<Sql_AuditLog>&);

signals:
    /* 发送服务器 */
    //选中设备发生变化
    void selectedDeviceChanged(int);
    
    //发送指令信号  命令+参数
    void sendCommand(QString& cmd,QString& param);

    //发送历史数据查询信号
    void sendQueryHistory(const QString& startTime, const QString& endTime);

    //发送安全审计查询信号
    void sendQueryLog(const QString& type);
    

    /* 发送本地 */

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();


private:
    Ui::MainWindow *ui;

    /* 标志位 */
    QDateTime objectstartTime;  //程序启动时间
    

};

