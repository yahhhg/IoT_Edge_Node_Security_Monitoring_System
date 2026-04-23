import random

#温湿度传感器模拟
class DHT22Sensor:
    def __init__(self, simulation_mode=True):   # 模式配置：True=模拟无硬件模式，False=真实硬件模式
       
        self.simulation_mode = simulation_mode
        
        # 模拟数据初始值（符合室内正常环境）
        self.current_temp = 25.0    # 初始温度25℃
        self.current_hum = 50.0     # 初始湿度50%RH
        
        # 数值合理范围（防止越界，符合传感器物理特性）
        self.temp_min = 18.0
        self.temp_max = 38.0
        self.hum_min = 20.0
        self.hum_max = 90.0
        
        # 单次最大变化步长（控制缓慢变化，避免跳变）
        self.temp_step = 0.3
        self.hum_step = 0.8

    def read(self):
        """读取传感器数据，返回(温度, 湿度)"""
        if self.simulation_mode:
            # 温度：在当前值基础上小幅度随机增减
            temp_change = random.uniform(-self.temp_step, self.temp_step)
            self.current_temp += temp_change
            # 限制在合理范围
            self.current_temp = max(self.temp_min, min(self.temp_max, self.current_temp))

            # 湿度：在当前值基础上小幅度随机增减
            hum_change = random.uniform(-self.hum_step, self.hum_step)
            self.current_hum += hum_change
            self.current_hum = max(self.hum_min, min(self.hum_max, self.current_hum))

            return round(self.current_temp, 1), round(self.current_hum, 1)
        
        # 预留真实硬件读取接口，后续接硬件时补充即可
        else:
            return None, None