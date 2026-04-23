#include "stdafx.h"
#include "MainWindow.h"

#include <QFile>           //文件读取
#include <QFileDialog>     //文件对话框，用于选择文件

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow())
{
    ui->setupUi(this);

    /* 标志位初始化 */
    //记录程序启动时间
    objectstartTime= QDateTime::currentDateTime();

    /* UI操作 */
    //实例化状态右边的永久标签
    statusLabel = new QLabel("未启动   ");
    ui->statusbar->addPermanentWidget(statusLabel);
    //设置一些组件初始不可用
    ui->sendCommandBtn->setEnabled(false);
    //历史查询时间设置
    ui->startDateTimeEdit->setDateTime(objectstartTime);
    ui->endDateTimeEdit->setDateTime(objectstartTime.addSecs(10));
    //便利视图 QTableWidget 的设置
    //设置固定列数
    ui->historyTableWidget->setColumnCount(5);
    ui->logTableWidget->setColumnCount(3);
    //设置初始行数
    ui->historyTableWidget->setRowCount(0);
    ui->logTableWidget->setRowCount(0);
    //设置表头
    ui->historyTableWidget->setHorizontalHeaderLabels(QStringList() << "时间" << "设备IP和端口" << "温度"<<"湿度"<<"气体");
    ui->logTableWidget->setHorizontalHeaderLabels(QStringList() << "时间" << "内容" << "是否成功");
    //所有列均匀拉伸，100%填满表格宽度，无任何留白
    ui->historyTableWidget->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    ui->logTableWidget->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    //初始隐藏表头
    ui->historyTableWidget->horizontalHeader()->setVisible(false);
    ui->logTableWidget->horizontalHeader()->setVisible(false);
  

    /* 对象初始化 */
    //实例化服务器控制器
    serverMgr = new TcpServerMgr(this);
   


    /* 信号与槽连接 */
    //新连接更新设备管理列表
    connect(serverMgr, &TcpServerMgr::updateListUI, this, &MainWindow::do_updateListUI);
    //当前设备管理列表中若选中项改变
    connect(this,&MainWindow::selectedDeviceChanged, serverMgr, &TcpServerMgr::onSelectedDeviceChanged);
    //发送指令信号
    connect(this, &MainWindow::sendCommand, serverMgr, &TcpServerMgr::onCmdFromUI);
    //打印下位机返回信息
    connect(serverMgr, &TcpServerMgr::printToUI, this, &MainWindow::do_printInfo);
    //更新传感器数据
    connect(serverMgr, &TcpServerMgr::updateSensorValueToUI, this, &MainWindow::do_updateSensorState);
    //查询历史数据
    connect(this,&MainWindow::sendQueryHistory, serverMgr, &TcpServerMgr::onQueryHistoryData);
    //接收历史数据查询信息
    connect(serverMgr, &TcpServerMgr::sendHistoryDataToUI, this, &MainWindow::do_receiveQueryHistory);
    //查询安全审计信息
    connect(this, &MainWindow::sendQueryLog, serverMgr, &TcpServerMgr::onQuerySecurityLog);
    //接收安全审计信息
    connect(serverMgr, &TcpServerMgr::sendSecurityLogToUI, this, &MainWindow::do_receiveQueryLog);

}

MainWindow::~MainWindow()
{
    delete ui;
}

//启动监控按钮按下槽
void MainWindow::on_toggleMonitorAct_triggered(bool checked)
{
    //更新UI
    ui->toggleMonitorAct->setText(checked ? "关闭监控" : "启动监控");
    statusLabel->setText(checked ? "已启动   " : "未启动   ");
    ui->refreshDeviceAct->setEnabled(checked);

    if (checked) {
        //开始监听 
        if (serverMgr->startListen(8888))
        {
            ui->statusbar->showMessage("监听已启动", 2000);
            
        }
        else
        {
            //监听失败，恢复UI
            ui->toggleMonitorAct->setText("启动监控");
            statusLabel->setText("未启动   ");
            ui->statusbar->showMessage("监听失败！", 2000);          //显示暂时信息

        }
    }
    else 
    {
        //停止监听
        serverMgr->closeListen();
        //监听关闭，恢复UI
        ui->toggleMonitorAct->setText("启动监控");
        statusLabel->setText("未启动   ");
        ui->statusbar->showMessage("监听已关闭", 2000);              //显示暂时信息
    }
}

//刷新设备按钮按下槽
void MainWindow::on_refreshDeviceAct_triggered()
{
    //设置启动关闭服务器按钮不可用
    ui->toggleMonitorAct->setEnabled(false);

    //关闭服务器
    serverMgr->closeListen();

    /* 不堵塞UI的等待,保证服务器关闭 */
    //创建事件循环
    QEventLoop loop;
    //单次定时器 ，1秒后退出事件循环
    QTimer::singleShot(1000, &loop, &QEventLoop::quit); // 1秒超时保护
    //开始事件循环 主线程会在这里 “暂停” 等待，进入一个内部的消息循环 但Qt 的事件机制依然在运行，界面保持响应
    loop.exec();

    //重新启动服务器
    serverMgr->startListen(8888);

    //设置启动关闭服务器按钮可用
    ui->toggleMonitorAct->setEnabled(true);
}

//处理报警
void MainWindow::do_handleAlarm(int alarmValue)
{
    switch (alarmValue) 
    {
    //温度报警
    case 4:    
        ui->statusbar->showMessage("温度报警！", 2000);
        ui->isAlarmLabel->setStyleSheet("color: red;");
        ui->tempValueLabel->setStyleSheet("color: red;");
        DatabaseManager::getInstance().insertAuditLog("报警事件", "温度超出阈值");
        break;
    //湿度报警
    case 2:
        ui->statusbar->showMessage("湿度报警！", 2000);
        ui->isAlarmLabel->setStyleSheet("color: red;");
        ui->humidityValueLabel->setStyleSheet("color: red;");
        DatabaseManager::getInstance().insertAuditLog("报警事件", "湿度超出阈值");
        break;
    //烟雾报警
    case 1:
        ui->statusbar->showMessage("烟雾报警！", 2000);
        ui->isAlarmLabel->setStyleSheet("color: red;");
        ui->gasValueLabel->setStyleSheet("color: red;");
        DatabaseManager::getInstance().insertAuditLog("报警事件", "烟雾浓度超出阈值");
        break;
    //全部报警
    case 7:
        ui->statusbar->showMessage("全部报警！", 2000);
        ui->isAlarmLabel->setStyleSheet("color: red;");
        ui->tempValueLabel->setStyleSheet("color: red;");
        ui->humidityValueLabel->setStyleSheet("color: red;");
        ui->gasValueLabel->setStyleSheet("color: red;");
        DatabaseManager::getInstance().insertAuditLog("报警事件", "温度、湿度、烟雾浓度超出阈值");
        break;
    //湿度+烟雾报警
    case 3:
        ui->statusbar->showMessage("湿度+烟雾报警！", 2000);
        ui->isAlarmLabel->setStyleSheet("color: red;");
        ui->humidityValueLabel->setStyleSheet("color: red;");
        ui->gasValueLabel->setStyleSheet("color: red;");
        DatabaseManager::getInstance().insertAuditLog("报警事件", "湿度、烟雾浓度超出阈值");
        break;
    //温度+烟雾报警
    case 5:
        ui->statusbar->showMessage("温度+烟雾报警！", 2000);
        ui->isAlarmLabel->setStyleSheet("color: red;");
        ui->tempValueLabel->setStyleSheet("color: red;");
        ui->gasValueLabel->setStyleSheet("color: red;");
        DatabaseManager::getInstance().insertAuditLog("报警事件", "温度、烟雾浓度超出阈值");
        break;
    //温度+湿度报警
    case 6:
        ui->statusbar->showMessage("温度+湿度报警！", 2000);
        ui->isAlarmLabel->setStyleSheet("color: red;");
        ui->tempValueLabel->setStyleSheet("color: red;");
        ui->humidityValueLabel->setStyleSheet("color: red;");
        DatabaseManager::getInstance().insertAuditLog("报警事件", "温度、湿度超出阈值");
        break;
    //无报警
    default:
        ui->isAlarmLabel->setStyleSheet("color: white;");
        ui->tempValueLabel->setStyleSheet("color: white;");
        ui->humidityValueLabel->setStyleSheet("color: white;");
        ui->gasValueLabel->setStyleSheet("color: white;");
    }
}


//发送指令按钮按下槽
void MainWindow::on_sendCommandBtn_clicked()
{
    /* 读取界面上的输入内容 */ 
    QString selectedCommand = ui->commandCombo->currentText();       // 读取下拉框选中的文本
    QString paramInput = ui->paramLineEdit->text().trimmed();        // 读取参数输入框的内容  

    //包装为信号发送给服务器控制器
    emit sendCommand(selectedCommand, paramInput);

    // 发送完清空参数输入框，方便下次输入
    ui->paramLineEdit->clear();
}

//设备列表项选中槽
void MainWindow::on_deviceListWidget_itemSelectionChanged()
{
    //获取当前选中的设备ID 和名称
    int deviceID = ui->deviceListWidget->currentRow();
    QString deviceName = ui->deviceListWidget->currentItem()->text();
    
    //更新UI
    do_updateListUI(deviceName, deviceID);
    //重置图表
    ui->chartPlaceholderWidget->clearChart();

    //发送信号给服务器控制器
    emit selectedDeviceChanged(deviceID);
}

//清空编辑框按钮按下槽
void MainWindow::on_clearEditBtn_clicked()
{
    ui->outputTextEdit->clear();
}

//历史数据查询按钮按下槽
void MainWindow::on_queryHistoryBtn_clicked()
{
    // 清空表格
    ui->historyTableWidget->setRowCount(0);

    //获取编辑框的时间
    QDateTime startTime = ui->startDateTimeEdit->dateTime();
    QDateTime endTime = ui->endDateTimeEdit->dateTime();

    // 核心校验逻辑
    bool isParamsValid = true;
    QString errorMsg;

    // 开始时间不能早于程序启动时间
    if (startTime < objectstartTime) {
        isParamsValid = false;
        errorMsg = "❌ 开始时间不能早于当前时间！";
        //自动修正为当前时间（如果不想弹窗提示）
        ui->startDateTimeEdit->setDateTime(objectstartTime);
        startTime = objectstartTime;
    }

    // 结束时间必须晚于开始时间（避免时间范围倒序）
    if (endTime < startTime) {
        isParamsValid = false;
        errorMsg = "❌ 结束时间不能早于开始时间！";
    }


    // 校验不通过时弹窗提示并终止后续操作
    if (!isParamsValid) {
        QMessageBox::warning(this, "参数错误", errorMsg);
        return; // 不执行后面的emit，避免发送错误数据给服务器
    }

    // 转换为信号需要的字符串格式
    QString startTimeStr = startTime.toString("yyyy-MM-dd HH:mm:ss");
    QString endTimeStr = endTime.toString("yyyy-MM-dd HH:mm:ss");

    // 发送信号给服务器
    emit sendQueryHistory(startTimeStr, endTimeStr);
}

//安全审计查询按下槽
void MainWindow::on_queryLogBtn_clicked()
{
    // 清空表格
    ui->logTableWidget->setRowCount(0);

    //获取选择
    QString selectedLogType = ui->logTypeCombo->currentText();

    //发送信号给服务器
    emit sendQueryLog(selectedLogType);
}

//一键导出历史数据菜单项
void MainWindow::on_exportDataAct_triggered()
{
    // 默认文件名是 "历史数据_当前时间.csv"，默认保存到桌面
    QString defaultFileName = QString("历史数据_%1.csv")
        .arg(QDateTime::currentDateTime().toString("yyyyMMdd_HHmmss"));
    QString filePath = QFileDialog::getSaveFileName(
        this,
        "导出历史数据",
        QStandardPaths::writableLocation(QStandardPaths::DesktopLocation) + "/" + defaultFileName,
        "CSV 文件 (*.csv)"
    );

    // 如果点击了取消，直接返回
    if (filePath.isEmpty()) {
        return;
    }

    // 获取要导出的数据 所有数据
    QString startTime = "2000-01-01 00:00:00";
    QString endTime = "2099-12-31 23:59:59";
    QList<Sql_SensorData> dataList = DatabaseManager::getInstance().querySensorDataByTimeRange(startTime, endTime);

    // 如果没有数据，提示
    if (dataList.isEmpty()) {
        QMessageBox::information(this, "提示", "没有可导出的历史数据");
        return;
    }

    // 写入 CSV 文件
    QFile file(filePath);
    // 以只读写入模式打开文件，有则修改，无则创建
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QMessageBox::critical(this, "错误", "无法打开文件进行写入，请检查文件权限");
        return;
    }

    // 创建文本流对象，用于写入内容
    QTextStream out(&file);
    // 设置编码为 UTF-8 并添加 BOM 头，防止 Excel 打开中文乱码
    out.setEncoding(QStringConverter::Utf8);
    out << QString::fromUtf8("\xEF\xBB\xBF");

    // 先写 CSV 表头（第一行）
    out << "时间,温度(℃),湿度(%RH),气体浓度\n";

    // 循环写入每一行数据
    for (const Sql_SensorData& data : dataList) {
        // 每一行的字段用逗号分隔，最后加换行符
        out << QString("%1,%2,%3,%4\n")
            .arg(data.timestamp)
            .arg(data.temperature, 0, 'f', 1)
            .arg(data.humidity, 0, 'f', 1)
            .arg(data.gas_concentration, 0, 'f', 1);
    }

    //关闭文件，有开就有关
    file.close();

    // 提示用户导出成功
    QMessageBox::information(this, "成功", QString("历史数据已成功导出到：\n%1").arg(filePath));
}

//一键导出安全审计数据菜单项
void MainWindow::on_exportLogAct_triggered() 
{
    // 默认文件名是 "安全审计_当前时间.csv"，默认保存到桌面
    QString defaultFileName = QString("安全审计_%1.csv")
        .arg(QDateTime::currentDateTime().toString("yyyyMMdd_HHmmss"));
    QString filePath = QFileDialog::getSaveFileName(
        this,
        "导出安全审计数据",
        QStandardPaths::writableLocation(QStandardPaths::DesktopLocation) + "/" + defaultFileName,
        "CSV 文件 (*.csv)"
    );

    // 如果点击了取消，直接返回
    if (filePath.isEmpty()) {
        return;
    }

    // 获取要导出的数据 所有数据
    QList<Sql_AuditLog> dataList = DatabaseManager::getInstance().queryAuditLogByType("全部");

    // 如果没有数据，提示
    if (dataList.isEmpty()) {
        QMessageBox::information(this, "提示", "没有可导出的安全审计数据");
        return;
    }

    // 写入 CSV 文件
    QFile file(filePath);
    // 以只读写入模式打开文件，有则修改，无则创建
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QMessageBox::critical(this, "错误", "无法打开文件进行写入，请检查文件权限");
        return;
    }

    // 创建文本流对象，用于写入内容
    QTextStream out(&file);
    // 设置编码为 UTF-8 并添加 BOM 头，防止 Excel 打开中文乱码
    out.setEncoding(QStringConverter::Utf8);
    out << QString::fromUtf8("\xEF\xBB\xBF");

    // 先写 CSV 表头（第一行）
    out << "时间,操作类型,操作内容,操作结果\n";
    // 循环写入每一行数据
    for (const Sql_AuditLog& data : dataList) {
        // 每一行的字段用逗号分隔，最后加换行符
        out << QString("%1,%2,%3,%4\n")
            .arg(data.operationTime)
            .arg(data.operationType)
            .arg(data.operationContent)
            .arg(data.operationResult);
    }

    //关闭文件，有开就有关
    file.close();

    // 提示用户导出成功
    QMessageBox::information(this, "成功", QString("安全审计数据已成功导出到：\n%1").arg(filePath));
}

//退出系统菜单项
void MainWindow::on_exitAct_triggered()
{
    QCoreApplication::quit();         //退出应用程序
}

//隐藏设备面板菜单项
void MainWindow::on_toggleDevicePanelAct_triggered(bool checked)
{
    ui->deviceDockWidget->setVisible(!checked);
}

//隐藏菜单面板菜单项
void MainWindow::on_toggleControlPanelAct_triggered(bool checked)
{
    ui->controlDockWidget->setVisible(!checked);
}

//关于系统菜单项
void MainWindow::on_aboutAct_triggered()
{
    QMessageBox::aboutQt(this);
}

//一键导出报表菜单项
void MainWindow::on_exportReportAct_triggered()
{
    on_exportDataAct_triggered();
    on_exportLogAct_triggered();
}

//新连接时更新设备管理列表
void MainWindow::do_updateListUI(QString deviceName, int deviceID)
{
    //若设备ID为-1，则说明服务器关闭
    if (deviceID == -1) 
    {
       //清空设备列表
       ui->deviceListWidget->clear();
       //更新UI
       ui->deviceIdLabel->setText("设备ID：未选择");             //更新设备ID
       ui->lastUpdateLabel->setText("最后更新：--");             //更新最后更新时间
       ui->networkStatusLabel->setText("网络状态：未连接");      //更新网络状态
       return;
    }

    //若设备名称为空，则说明客户端断开连接，从设备列表中移除该设备
    if (deviceName == "") 
    {
        //移除设备
        ui->deviceListWidget->removeItemWidget(ui->deviceListWidget->item(deviceID));
        //更新UI
        ui->deviceIdLabel->setText("设备ID：未选择");             //更新设备ID
        ui->lastUpdateLabel->setText("最后更新：--");             //更新最后更新时间
        ui->networkStatusLabel->setText("网络状态：未连接");      //更新网络状态
        return;
    }

    //若当前设备列表没有同类项
    if (ui->deviceListWidget->findItems(deviceName, Qt::MatchExactly).isEmpty())
        ui->deviceListWidget->addItem(deviceName);                        //添加设备到设备列表
    ui->deviceIdLabel->setText("设备ID：" + QString::number(deviceID));   //更新设备ID
    ui->lastUpdateLabel->setText("最后更新：" + QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss")); //更新最后更新时间
    ui->networkStatusLabel->setText("网络状态：已连接");                  //更新网络状态
    ui->sendCommandBtn->setEnabled(true);                                 //启用发送指令按钮

    //如果当前设备列表只有一个则自动选择该设备
    if (ui->deviceListWidget->count() == 1)
        ui->deviceListWidget->setCurrentRow(0);
}

//打印下位机返回信息
void MainWindow::do_printInfo(QString info)
{
    //文本框追加信息
    ui->outputTextEdit->append(info);
}

//更新设备传感器状态
void MainWindow::do_updateSensorState(const SensorData& data)
{
    //更新状态值
    ui->tempValueLabel->setText(QString::number(data.temperature)+" ℃");           //更新温度
    ui->humidityValueLabel->setText(QString::number(data.humidity)+" %RH");         //更新湿度
    ui->gasValueLabel->setText(QString::number(data.gas_concentration)+" ppm");     //更新气体浓度
    ui->isAlarmLabel->setText(data.is_alarm==0 ? "正常" : "不正常");                //更新是否报警

    //处理报警状态
    do_handleAlarm(data.is_alarm);

    //更新图表 调用提供的接口
    ui->chartPlaceholderWidget->addSensorData(data.temperature, data.humidity, data.gas_concentration);

}

//更新历史数据查询结果
void MainWindow::do_receiveQueryHistory(QList<Sql_SensorData>& dataList)
{
    //显示表头
    ui->historyTableWidget->horizontalHeader()->setVisible(true);

    //遍历数据
    foreach(auto data,dataList) {
        //添加一行的数据
        ui->historyTableWidget->insertRow(ui->historyTableWidget->rowCount());
        ui->historyTableWidget->setItem(ui->historyTableWidget->rowCount() - 1, 0, new QTableWidgetItem(data.timestamp));
        ui->historyTableWidget->setItem(ui->historyTableWidget->rowCount() - 1, 1, new QTableWidgetItem(data.ip_port));
        ui->historyTableWidget->setItem(ui->historyTableWidget->rowCount() - 1, 2, new QTableWidgetItem(QString::number(data.temperature)));
        ui->historyTableWidget->setItem(ui->historyTableWidget->rowCount() - 1, 3, new QTableWidgetItem(QString::number(data.humidity)));
        ui->historyTableWidget->setItem(ui->historyTableWidget->rowCount() - 1, 4, new QTableWidgetItem(QString::number(data.gas_concentration)));
    }
}

//更新安全审计数据接收
void MainWindow::do_receiveQueryLog(QList<Sql_AuditLog>& dataList)
{
    //显示表头
    ui->logTableWidget->horizontalHeader()->setVisible(true);

    //遍历数据
    foreach(auto data, dataList) {
        //添加一行的数据
        ui->logTableWidget->insertRow(ui->logTableWidget->rowCount());
        ui->logTableWidget->setItem(ui->logTableWidget->rowCount() - 1, 0, new QTableWidgetItem(data.operationTime));
        ui->logTableWidget->setItem(ui->logTableWidget->rowCount() - 1, 1, new QTableWidgetItem(data.operationContent));
        ui->logTableWidget->setItem(ui->logTableWidget->rowCount() - 1, 2, new QTableWidgetItem(data.operationResult));
    }
}