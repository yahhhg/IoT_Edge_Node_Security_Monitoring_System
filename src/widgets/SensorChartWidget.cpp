#include "SensorChartWidget.h"
#include <QPainter>
#include <QPen>
#include <QFont>

// 构造函数：修复之前的基类错误，初始化计数，调用初始化逻辑
SensorChartWidget::SensorChartWidget(QWidget* parent)
    : QChartView(parent)
    , m_pointCount(0)                // 初始化数据点计数为0
{
    initChart();                     // 先初始化图表基础框架
    initSeries();                    // 再初始化曲线和坐标轴
}

// 析构函数：Qt父子机制自动回收内存，无需手动delete
SensorChartWidget::~SensorChartWidget()
{
    delete m_chart;
}

// 对外核心接口：传入传感器数据，更新曲线+实现滚动效果
void SensorChartWidget::addSensorData(double temp, double hum, double gas)
{
    // 给三条曲线添加新数据点  为 X轴值，Y轴值
    m_tempSeries->append(m_pointCount, temp);
    m_humSeries->append(m_pointCount, hum);
    m_gasSeries->append(m_pointCount, gas);

    // 数据点计数自增
    m_pointCount++;

    // 实现曲线滚动：超过最大点数，删除最旧的点
    if (m_tempSeries->count() > m_maxPoints)
    {
        // 三条曲线同步删除最旧的第0个点 即位于第一个的点
        m_tempSeries->remove(0);
        m_humSeries->remove(0);
        m_gasSeries->remove(0);

        // X轴范围跟着滚动，始终显示最新的50个点
        m_axisX->setRange(m_pointCount - m_maxPoints, m_pointCount);
    }
    // 没超过最大点数时，X轴范围固定
    else
    {
        m_axisX->setRange(0, m_maxPoints);
    }
}

// 清空图表所有数据，重置状态
void SensorChartWidget::clearChart()
{
    // 清空三条曲线的所有数据点
    if (m_tempSeries) {
        m_tempSeries->clear();
    }
    if (m_humSeries) {
        m_humSeries->clear();
    }
    if (m_gasSeries) {
        m_gasSeries->clear();
    }

    // 重置数据点计数
    m_pointCount = 0;

    // 重置X轴范围，回到初始状态
    if (m_axisX) {
        m_axisX->setRange(0, m_maxPoints);
    }

    // 刷新图表显示，确保界面立刻更新
    if (m_chart) {
        m_chart->update();
    }
}

// 初始化图表基础配置
void SensorChartWidget::initChart()
{
    // 创建图表核心对象，绑定父对象自动管理内存
    m_chart = new QChart();
    m_chart->setTitle("实时传感器数据监控曲线");
    m_chart->setTitleFont(QFont("Microsoft YaHei", 10, QFont::Bold));        // 设置标题字体

    //设置图表内边距
    m_chart->setMargins(QMargins(0,0,0,0));

    // 配置图例，这是用于提示三条曲线的
    m_chart->legend()->setVisible(true);
    m_chart->legend()->setAlignment(Qt::AlignBottom); // 图例放在底部
    m_chart->legend()->setFont(QFont("Microsoft YaHei", 10));

    // 把图表绑定到当前控件
    this->setChart(m_chart);
    // 开启抗锯齿，曲线更平滑
    this->setRenderHint(QPainter::Antialiasing);
    // 禁止鼠标误拖拽图表
    this->setDragMode(QChartView::NoDrag);
}

// 初始化曲线、坐标轴
void SensorChartWidget::initSeries()
{
    /* 创建三条曲线，设置名称、颜色、线条样式 */
    // 温度曲线：红色，线宽2px
    m_tempSeries = new QSplineSeries(m_chart);
    m_tempSeries->setName("温度(℃)");
    m_tempSeries->setColor(QColor(255, 60, 60));
    m_tempSeries->setPen(QPen(m_tempSeries->color(), 2));

    // 湿度曲线：蓝色，线宽2px
    m_humSeries = new QSplineSeries(m_chart);
    m_humSeries->setName("湿度(%RH)");
    m_humSeries->setColor(QColor(60, 120, 255));
    m_humSeries->setPen(QPen(m_humSeries->color(), 2));

    // 气体浓度曲线：绿色，线宽2px
    m_gasSeries = new QSplineSeries(m_chart);
    m_gasSeries->setName("气体浓度");
    m_gasSeries->setColor(QColor(60, 200, 60));
    m_gasSeries->setPen(QPen(m_gasSeries->color(), 2));

    
    /* 把曲线添加到图表中 */ 
    m_chart->addSeries(m_tempSeries);
    m_chart->addSeries(m_humSeries);
    m_chart->addSeries(m_gasSeries);

    /* 创建X轴：数据点序号，固定显示最近50个点 */ 
    m_axisX = new QValueAxis(m_chart);
    m_axisX->setTitleText("数据点序号");
    m_axisX->setLabelFormat("%d"); // 显示整数
    m_axisX->setRange(0, m_maxPoints);
    m_axisX->setTickCount(6); // 刻度数量
    m_chart->addAxis(m_axisX, Qt::AlignBottom); // X轴固定在底部

    /* 创建3个独立Y轴，适配不同传感器的数值范围 */ 
    // 温度Y轴：左侧1，范围0-60℃
    m_axisYTemp = new QValueAxis(m_chart);
    m_axisYTemp->setTitleText("温度(℃)");
    m_axisYTemp->setLabelFormat("%.1f");
    m_axisYTemp->setRange(0, 60);
    m_axisYTemp->setLinePenColor(m_tempSeries->color()); // 轴颜色和曲线一致
    m_axisYTemp->setTitleBrush(m_tempSeries->color());
    m_chart->addAxis(m_axisYTemp, Qt::AlignLeft);        // 将坐标轴加入图表，指定位置 

    // 湿度Y轴：左侧2，范围0-100%RH
    m_axisYHum = new QValueAxis(m_chart);
    m_axisYHum->setTitleText("湿度(%RH)");
    m_axisYHum->setLabelFormat("%.1f");
    m_axisYHum->setRange(0, 100);
    m_axisYHum->setLinePenColor(m_humSeries->color());
    m_axisYHum->setTitleBrush(m_humSeries->color());
    m_chart->addAxis(m_axisYHum, Qt::AlignLeft);

    // 气体浓度Y轴：右侧，范围0-200
    m_axisYGas = new QValueAxis(m_chart);
    m_axisYGas->setTitleText("气体浓度(ppm)");
    m_axisYGas->setLabelFormat("%.1f");
    m_axisYGas->setRange(0, 100);
    m_axisYGas->setLinePenColor(m_gasSeries->color());
    m_axisYGas->setTitleBrush(m_gasSeries->color());
    m_chart->addAxis(m_axisYGas, Qt::AlignRight);

    /* 曲线和坐标轴绑定 */ 
    //为不同曲线分别绑定X轴和Y轴
    m_tempSeries->attachAxis(m_axisX);
    m_tempSeries->attachAxis(m_axisYTemp);

    m_humSeries->attachAxis(m_axisX);
    m_humSeries->attachAxis(m_axisYHum);

    m_gasSeries->attachAxis(m_axisX);
    m_gasSeries->attachAxis(m_axisYGas);
}

