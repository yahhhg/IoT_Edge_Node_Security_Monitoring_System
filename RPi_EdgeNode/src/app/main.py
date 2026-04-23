import sys
from pathlib import Path
# 把项目根目录加入Python搜索路径
sys.path.append(str(Path(__file__).parent.parent.parent))

from src.network.tcp_client import TCPClientTool  # 导入封装好的TCP客户端工具类

# 程序主入口
def main():
    # 创建客户端实例
    client = TCPClientTool()
    # 启动客户端（自动连接、心跳、重连）
    client.run()


if __name__ == "__main__":
   main()