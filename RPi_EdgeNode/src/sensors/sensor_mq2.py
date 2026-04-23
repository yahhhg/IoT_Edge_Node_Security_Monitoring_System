import random

#烟雾传感器模拟
class MQ2Sensor:
    def __init__(self, simulation_mode=True):   # 模式配置：True=模拟无硬件模式，False=真实硬件模式
        
        self.simulation_mode = simulation_mode
        
        # 模拟数据初始值（正常洁净空气数值）
        self.current_gas = 10.0
        
        # 数值合理范围
        self.gas_min = 0.0
        self.gas_max = 200.0  # 超过100视为异常
        
        # 单次最大变化步长
        self.gas_step = 1.0

    def read(self):
        """读取传感器数据，返回(烟雾浓度值, 是否超标)"""
        if self.simulation_mode:
            # 浓度值：在当前值基础上小幅度随机增减
            gas_change = random.uniform(-self.gas_step, self.gas_step)
            self.current_gas += gas_change
            # 限制在合理范围
            self.current_gas = max(self.gas_min, min(self.gas_max, self.current_gas))

            # 判断是否超标
            is_alarm = self.current_gas >= 100.0
            return round(self.current_gas, 1), is_alarm
        
        # 预留真实硬件读取接口，后续接硬件时补充即可
        else:
            return None, None