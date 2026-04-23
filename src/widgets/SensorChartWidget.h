#ifndef SENSORCHARTWIDGET_H
#define SENSORCHARTWIDGET_H

#if 0
    此类为传感器实时曲线绘制的自定义控件类
#endif

/* 绘制曲线图 */
#include <QChartView>      //图表视图
#include <QSplineSeries>     //平滑曲线图
#include <QValueAxis>      //数值坐标轴
#include <QChart>          //图表

class SensorChartWidget  : public QChartView
{
	Q_OBJECT

public:
	SensorChartWidget(QWidget *parent=nullptr);
	~SensorChartWidget();

	// 对外核心接口 ui调这个函数传数据
    void addSensorData(double temp, double hum, double gas);

    //清空图表
    void clearChart();

private:
    // 内部私有成员：3条曲线、图表、坐标轴、数据点计数
    QChart* m_chart;              //图表
    QSplineSeries* m_tempSeries;    //温度曲线
    QSplineSeries* m_humSeries;     //湿度曲线
    QSplineSeries* m_gasSeries;     //气体曲线
    QValueAxis* m_axisX;          //统一X轴
    QValueAxis* m_axisYTemp;      //温度Y轴
    QValueAxis* m_axisYHum;       //湿度Y轴
    QValueAxis* m_axisYGas;       //气体浓度Y轴

    int m_pointCount;             // 记录当前数据点数量
    const int m_maxPoints = 50;   // 最多显示50个点，超过就滚动

    // 内部私有函数：初始化图表、初始化曲线
    void initChart();
    void initSeries();
};

#endif // SENSORCHARTWIDGET_H