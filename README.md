# 基于树莓派的工业物联网边缘节点监控系统

# 环境要求

上位机（Windows 端）：

- 操作系统：Windows 10 / Windows 11（64 位）
- 开发环境：Qt 6.5+（推荐 LTS 版本，如 6.6.3 LTS）
- 编译器：MinGW 64-bit 或 MSVC 2019/2022 64-bit
- 依赖 Qt 模块：Core Gui Widgets Network Sql Charts        

下位机（树莓派端）：

- 硬件：树莓派全系列型号（推荐树莓派 3B+/4B/5）
- 操作系统：Raspberry Pi OS (Debian 11/12)、Ubuntu for ARM 等主流 Linux 发行版
- 软件环境：Python 3.8+
- 依赖 Python 库：
python3-pip / python3-venv：Python 环境与虚拟环境管理；
python3-libgpiod：树莓派 5 新一代 GPIO 内核驱动（替代旧版 wiringPi）；
i2c-tools / python3-smbus2：I2C 总线通信驱动（MQ2 + PCF8591 ADC 专用）；
adafruit-blinka：树莓派硬件抽象层，提供统一的硬件引脚 / 总线接口；
adafruit-circuitpython-dht：官方新版 DHT 传感器驱动（兼容树莓派 5）；
RPi.GPIO：GPIO 兼容辅助库；
adafruit-circuitpython-pcf8591：PCF8591 ADC 模块驱动（读取模拟量）；

# 核心参数配置

下位机 sensor_dht22.py 与 sensor_mq2.py 默认使用软件模拟硬件输出，修改其构造的def __init__ 参数simulation_mode=False 来使用真实硬件输出

# 核心功能：

本项目是一套面向工业物联网场景的轻量化监控系统，以树莓派为边缘采集终端，Qt/C++开发跨平台上位机监控中心，实现环境数据的端到端安全采集、智能处理、实时监控与合规审计，兼顾边缘响应能力与传输安全性，适配中小规模工业现场、机房、仓储等场景的环境安全监控需求。

- **边缘数据采集与智能预处理**
  支持温湿度、烟雾/可燃气体等工业环境参数的实时采集；边缘端本地完成数据滤波、阈值判断，异常时直接触发本地声光报警，减少上行传输压力；支持断网数据本地缓存、网络恢复后续传，保障数据不丢失。

- **多设备统一管理与实时可视化**
  支持多台树莓派边缘节点的统一接入、在线状态管理；基于Qt Charts实现环境数据的实时动态曲线与数值展示，直观呈现现场环境变化与设备运行状态。

- **数据持久化与历史回溯**
  内置SQLite轻量级数据库，实现全量监控数据、告警事件的持久化存储；支持按时间范围筛选、查询历史数据，可导出数据报表，满足事件追溯需求。

-  **异常告警与远程管控**
  实现上位机弹窗告警的告警机制；支持上位机远程下发指令，修改告警阈值、启停采集任务、远程重启设备。

- **全链路安全审计与合规管理**
  全量记录平台操作、设备上下线、告警事件等行为日志，日志不可篡改；支持日志查询、导出与完整性校验，满足网络安全与工业场景的合规审计要求。

# 项目结构：

### 上位机 (Qt/C++ HostComputer)

```text
HostComputer/
├── CMakeLists.txt           # CMake构建脚本，定义Qt6依赖（Widgets/Charts/Network/Sql）、源文件关联
├── downloads/               # 运行时文件接收目录，存放从树莓派下载的日志/缓存数据
└── src/                         # 项目源码
    ├── app/  
    │   └── main.cpp             # 应用入口，初始化Qt应用、加载主窗口
    ├── ui/                      # UI层：界面与资源
    │   └── MainWindow.ui        # Qt Designer主窗口布局文件
    ├── core/                    # 业务核心层：数据与逻辑
	│   ├── MainWindow.h         # 主窗口类声明（UI交互、信号槽绑定）
	│   ├── DatabaseManager.h    # 数据库管理类（SQLite存储/查询历史数据、审计日志）
	│   ├── MainWindow.cpp
	│   └── DatabaseManager.cpp
	├── network/                 # 通信层：TCP与安全传
	│   ├── TcpServerMgr.h       # TCP服务端管理（监听端口、管理多客户端接入）
	│   ├── TcpClientHandler.h   # 单客户端处理（TLS加密通信、文件传输、心跳保活）
	│   ├── TcpServerMgr.cpp
	│   └── TcpClientHandler.cpp
    ├── widgets/                 # 自定义控件的文件夹
	│   ├── SensorChartWidget.h  # 自定义的传感器实时曲线绘制类
    │   └── SensorChartWidget.cpp
	└── utils/                   # 工具层：通用辅助函数
    	├──  Utils.h             # 时间格式化、字符串处理、文件路径工具
    	└──  Utils.cpp
     
```

### 下位机 (Python RPi_EdgeNode)

```text
RPi_EdgeNode/
├── config/                     # 配置目录
│   └── config.ini              # 全局配置文件（服务器IP/端口、采集间隔、告警阈值、设备ID）
├── src/                         # 项目源码
│   ├── app/                     # 应用入口层
│   │   └── main.py              # 程序入口，初始化各模块、启动采集任务与通信服务
│   ├── sensors/                 # 传感器采集层
│   │   ├── sensor_dht22.py      # 温湿度传感器驱动，支持模拟数据/真实硬件读取
│   │   └── sensor_mq2.py        # 烟雾传感器驱动，支持模拟数据/真实硬件读取
│   ├── core/                    # 业务核心层：数据与逻辑
│   │   └── data_collector.py    # 数据采集调度、阈值判断、断网数据本地缓存逻辑
│   ├── network/                 # 通信层：TCP传输与文件交互
│   │   ├── tcp_client.py        # TCP客户端实现、心跳保活、断网续传
│   │   └── file_transfer.py     # 文件收发底层实现，支持向上位机上传缓存数据与日志
│   └── utils/                   # 工具层：通用辅助函数
│       └── common_utils.py      # 配置读写、日志记录、文件操作通用工具
└── requirements.txt             # Python依赖包列表，声明项目所需第三方库
```

- 未添加 _init_py 包初始化文件，不适配低版本 python
