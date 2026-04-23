cmake_minimum_required(VERSION 3.16)
project(IoT_Edge_Node_Security_Monitoring_System LANGUAGES CXX)

include(qt.cmake)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 设置项目根目录 从而可以使用从根目录开始的路径来导入头文件高级保存选项
include_directories(${PROJECT_SOURCE_DIR})

# 告诉Qt UIC工具，在这个目录下搜索.ui文件
# Qt UIC工具默认只在根目录搜索.ui文件，如果需要指定其他目录，需要手动设置
set(CMAKE_AUTOUIC_SEARCH_PATHS ${CMAKE_SOURCE_DIR}/src/ui)

find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS Core)
find_package(Qt${QT_VERSION_MAJOR}
    COMPONENTS
        Core
        Gui
        Widgets
        Network       #网络模块
        Sql           #数据库模块
        Charts        #图表模块
)
qt_standard_project_setup()

set(PROJECT_SOURCES
    src/app/main.cpp
    src/ui/MainWindow.ui
    #UI控制类
    src/core/MainWindow.h
    src/core/MainWindow.cpp

    #服务器控制器类
    src/network/TcpServerMgr.h
    src/network/TcpServerMgr.cpp

    #客户端处理类
    src/network/TcpClientHandler.h
    src/network/TcpClientHandler.cpp

    #图表自定义控件
    src/widgets/SensorChartWidget.h
    src/widgets/SensorChartWidget.cpp
    
    #数据库管理类
    src/core/DatabaseManager.h
    src/core/DatabaseManager.cpp

    IoT_Edge_Node_Security_Monitoring_System.rc
)

qt_add_executable(${PROJECT_NAME} ${PROJECT_SOURCES})

set_target_properties(${PROJECT_NAME}
    PROPERTIES
        WIN32_EXECUTABLE TRUE
)

target_link_libraries(${PROJECT_NAME}
    PUBLIC
        Qt::Core
        Qt::Gui
        Qt::Widgets
        Qt6::Network
        Qt6::Sql
        Qt6::Charts
)
target_precompile_headers(${PROJECT_NAME} PUBLIC stdafx.h)
