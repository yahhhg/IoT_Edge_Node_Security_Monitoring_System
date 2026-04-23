import sys
import os
import time
from datetime import datetime

from pathlib import Path
# 把项目根目录加入Python搜索路径
sys.path.append(str(Path(__file__).parent.parent))

#导入硬件读取模块
from sensors.sensor_dht22 import DHT22Sensor
from sensors.sensor_mq2 import MQ2Sensor
from utils.common_utils import get_config

class DataCollector:
    def __init__(self):
        # 读取配置
        self.config = get_config()
        self.sample_interval = self.config["collection"].getint("sample_interval", 5)
        self.temp_max = self.config["threshold"].getfloat("temp_max", 50)
        self.hum_max = self.config["threshold"].getfloat("hum_max", 80)
        
        # 实例化模拟传感器（默认无硬件模式）
        self.dht22 = DHT22Sensor(simulation_mode=True)
        self.mq2 = MQ2Sensor(simulation_mode=True)

    #更新配置信息，重新读取配置文件
    def update_config(self):
        self.config = get_config()
        self.config = get_config()
        self.sample_interval = self.config["collection"].getint("sample_interval", 5)
        self.temp_max = self.config["threshold"].getfloat("temp_max", 50)
        self.hum_max = self.config["threshold"].getfloat("hum_max", 80)


    #读取传感器数据并封装成统一的上报格式
    def get_sensor_data(self):
        """
        读取传感器数据并封装成统一的上报格式
        :return: 符合约定的JSON字典
        """
        # 读取模拟数据
        temp, hum = self.dht22.read()
        gas, gas_alarm = self.mq2.read()
        
        #判断是否需要报警
        is_alarm= 0
        if(temp > self.temp_max ):
            is_alarm += 4
        if( hum > self.hum_max):
            is_alarm += 2
        if(gas_alarm):
            is_alarm += 1

        # 封装成统一的JSON格式 包含温度，湿度，气体浓度，气体报警状态
        sensor_data = {
            "type": "sensor_data",  # 固定类型标识
            "data": {
                "temperature": temp,
                "humidity": hum,
                "gas_concentration": gas,
                "is_alarm": is_alarm
            }
        }
        return sensor_data