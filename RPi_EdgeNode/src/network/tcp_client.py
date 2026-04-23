import socket       #网络socket
import json         #JSON数据读写
import time         #时间,用于计算超时
import struct       #用于打包和解包数据
import select       #用于多路复用IO 检测是否有数据到达

import src.utils.common_utils as utils                #工具类
from src.core.data_collector import DataCollector     #硬件数据收集器

import configparser     #读取 config.txt 配置文件
import os               #用于返回当前工作目录

#客户端网络相关操作
class TCPClientTool:
    def __init__(self):
        
        config=utils.get_config()
        
        '''
        self.SERVER_IP = "192.168.140.16"
        self.SERVER_PORT = 8888
        self.HEARTBEAT_INTERVAL = 5
        self.TIMEOUT = 30
        self.CONNECTION_TIMEOUT = 180
        self.BUFFER_SIZE = 65536
        self.sock = None
        '''# 从配置中读取参数
        self.SERVER_IP = config.get("server", "SERVER_IP").strip('"')            # 上位机IP地址
        self.SERVER_PORT = config.getint("server", "SERVER_PORT")                # 端口
        self.HEARTBEAT_INTERVAL = config.getint("server", "HEARTBEAT_INTERVAL")  # 心跳保活检测间隔
        self.TIMEOUT = config.getint("server", "TIMEOUT")                        # 处理超时间隔
        self.CONNECTION_TIMEOUT = config.getint("server", "CONNECTION_TIMEOUT")  # 连接超时间隔
        self.BUFFER_SIZE = config.getint("server", "BUFFER_SIZE")                # 缓冲区大小'''

        # 实例化数据采集器
        self.data_collector = DataCollector()
        self.last_data_send_time = time.time()  # 记录上次发送数据的时间

        # 重置阈值
        utils.update_config(section="threshold", key="temp_max", value=40.0)
        utils.update_config(section="threshold", key="hum_max", value=80.0)

        print(self.SERVER_IP, self.SERVER_PORT, self.HEARTBEAT_INTERVAL, self.TIMEOUT, self.CONNECTION_TIMEOUT, self.BUFFER_SIZE)

    #长度前缀法 发送 长度包+JSON数据包
    def send_data(self, data):
        json_str = json.dumps(data)
        json_bytes = json_str.encode("utf-8")
        length_bytes = struct.pack(">I", len(json_bytes))
        self.sock.sendall(length_bytes + json_bytes)

    #长度前缀法 接收 长度包+JSON数据包
    def recv_data(self):
        try:
            #先在缓冲区中接收四个字节的长度前缀数据
            length_bytes = self.sock.recv(4)
            if not length_bytes:
                return None
            #使用 struct 模块将二进制字节流解包为整数
            length = struct.unpack(">I", length_bytes)[0]
            if length == 0:
                return None

            #循环读取JSON数据
            data_bytes = b""
            while len(data_bytes) < length:
                chunk = self.sock.recv(min(self.BUFFER_SIZE, length - len(data_bytes)))
                if not chunk:
                    return None
                data_bytes += chunk
            return json.loads(data_bytes.decode("utf-8"))
        except socket.timeout:
            raise
        except Exception as e:
            print(f"接收数据错误: {e}")
            return None    
    
    #处理命令
    def cmd_handler(self,data):
        # 解析上位机指令
        cmd_type = data.get("cmd", "")
        param = data.get("param", "")
        
        # 返回结果数据包
        result = {
            "type": "result",
            "cmd": cmd_type,
            "success": False,
            "data": "" }
                    
        try:
            # 处理：修改温度阈值
            if cmd_type == "set_temp_threshold":
            # 校验参数：必须是有效数字
                if not param:
                    result["data"] = "失败：温度阈值参数不能为空"
                    return result
            
                try:
                    new_temp_max = float(param)
                except ValueError:
                    result["data"] = "失败：温度阈值必须是有效数字"
                    return result
                # 修改配置文件
                config_success = utils.update_config(section="threshold", key="temp_max", value=new_temp_max)
                if not config_success:
                    result["data"] = "失败：配置文件写入失败"
                    return result
                # 修改成功更新阈值
                self.data_collector.update_config()

                # 返回成功结果
                result["success"] = True
                result["data"] = f"成功：温度阈值已更新为 {new_temp_max}℃"
                return result
            #  处理：修改湿度阈值 
            elif cmd_type == "set_hum_threshold":
                # 校验参数：必须是有效数字
                if not param:
                    result["data"] = "失败：湿度阈值参数不能为空"
                    return result
            
                try:
                    new_hum_max = float(param)
                except ValueError:
                    result["data"] = "失败：湿度阈值必须是有效数字"
                    return result

                #  修改配置文件
                config_success = utils.update_config(section="threshold", key="hum_max", value=new_hum_max)
                if not config_success:
                   result["data"] = "失败：配置文件写入失败"
                   return result

                # 实时更新运行中的阈值
                self.data_collector.update_config()
            
                # 返回成功结果
                result["success"] = True
                result["data"] = f"成功：湿度阈值已更新为 {new_hum_max}%RH"
                return result
                
                # 处理：重启采集程序 
            elif cmd_type == "restart_collect":
                # 重置采集器状态（传感器数值、采集计数）
                if self.data_collector:
                     # 重置传感器初始值
                    self.data_collector.dht22.current_temp = 25.0
                    self.data_collector.dht22.current_hum = 50.0
                    self.data_collector.mq2.current_gas = 10.0
                    # 重新加载配置文件
                    self.data_collector.update_config()
            
                    # 2. 返回成功结果
                    result["success"] = True
                    result["data"] = "成功：采集程序已重启，配置已重新加载"
                    return result

            # 未知指令处理 
            else:
                result["data"] = f"失败：未知指令 {cmd_type}"
                return result

        except Exception as e:
            # 捕获所有异常，避免程序崩溃
            result["data"] = f"执行异常：{str(e)}"
            return result
      

    #主连接循环
    def run(self):
        while True:
            #套接字置于空
            self.sock = None  
            #记录最后心跳计时 
            last_heartbeat_recv = time.time()
            last_heartbeat_send = time.time()
            try:
                # 创建socket对象
                self.sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
                # 设置超时时间
                self.sock.settimeout(self.CONNECTION_TIMEOUT)
                # 连接服务器
                self.sock.connect((self.SERVER_IP, self.SERVER_PORT))
                print(f"已连接到上位机")
                # 设置心跳保活检测间隔
                self.sock.setsockopt(socket.SOL_SOCKET, socket.SO_KEEPALIVE, 1)
                self.sock.setsockopt(socket.IPPROTO_TCP, socket.TCP_KEEPIDLE, self.HEARTBEAT_INTERVAL)
                #更新最后心跳时间  
                last_heartbeat_send = time.time()

                while True:
                    current_time = time.time()
                    #不断检测心跳包以及发送心跳包
                    if current_time - last_heartbeat_send > self.HEARTBEAT_INTERVAL:
                        self.send_data({"type": "heartbeat", "data": "ping"})
                        last_heartbeat_send = current_time
                    if current_time - last_heartbeat_recv > self.CONNECTION_TIMEOUT:
                        print("长时间无响应，断开")
                        break
                    

                    # 定时采集并发送传感器数据
                    if current_time - self.last_data_send_time > self.data_collector.sample_interval:
                        # 从采集器获取封装好的数据 JSON格式
                        sensor_data = self.data_collector.get_sensor_data()
                        # 长度前缀法+JSON 发送
                        self.send_data(sensor_data)
                        # 更新发送时间
                        self.last_data_send_time = current_time
                        print(f"已上报传感器数据: {sensor_data['data']}")

                    #socket 非阻塞可读检测 若当前无数据可读则跳过 等待下一次循环
                    readable, _, _ = select.select([self.sock], [], [], 0.1)
                    if not readable:
                        continue

                    # 接收数据  
                    try:
                        data = self.recv_data() 
                    except socket.timeout:
                        continue

                    #若可读但读取数据为空 说明连接断开
                    if data is None :
                        print("连接断开")
                        break

                    if data:
                        # 若为命令则处理命令，并返回处理结果
                        if data["type"] == "cmd":
                            self.send_data(self.cmd_handler(data))
                        #若是心跳包，重置接收心跳计时   
                        elif data["type"] == "heartbeat":
                            last_heartbeat_recv = time.time()
                

            except Exception as e:
                print(f"异常: {e}，5秒后重连")
            finally:
                if self.sock:
                    try:
                        self.sock.shutdown(socket.SHUT_RDWR)
                    except:
                        pass
                    self.sock.close()
                time.sleep(5)

       