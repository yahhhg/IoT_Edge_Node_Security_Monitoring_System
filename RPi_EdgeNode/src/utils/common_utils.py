# src/utils/common_utils.py
import configparser
from pathlib import Path

# 获取配置文件
def get_config():
    """读取并返回config.ini配置对象"""
    config = configparser.ConfigParser()
    # 用Path替代os.path，避免路径错误
    config_path = Path(__file__).parent.parent.parent / "config" / "config.ini"
    
    # 调试：打印路径和文件状态
    print("配置文件路径:", config_path)
    print("文件是否存在:", config_path.exists())
    
    config.read(config_path, encoding="utf-8")
    return config

# 修改config.ini配置文件的接口
def update_config(section: str, key: str = None, value: str = None, data_dict: dict = None, encoding: str = "utf-8") -> bool:
    """
    section: 配置段名称
    key: 单个修改时：配置项的key
    value: 单个修改时：配置项的value（数字/布尔值会自动转成字符串）
    data_dict: 批量修改时：传入字典 {key1: value1, key2: value2}，优先级高于单个key/value
    encoding: 文件编码，默认utf-8
    return: 成功返回True，失败返回False
    """
    # 和get_config完全一致的路径，避免路径错误
    config_path = Path(__file__).parent.parent.parent / "config" / "config.ini"

    # 校验参数合法性
    if not config_path.exists():
        print(f"错误：配置文件不存在，路径：{config_path}")
        return False
    if key is None and data_dict is None:
        print("错误：必须传入key+value 或 data_dict批量修改字典")
        return False

    try:
        # 先读取现有配置）
        config = configparser.ConfigParser()
        config.read(config_path, encoding=encoding)

        # 批量修改模式
        if data_dict is not None and isinstance(data_dict, dict):
            for k, v in data_dict.items():
                config[section][k] = str(v)  # 所有值统一转字符串，符合ini规范
            print(f"批量修改配置 [{section}] 成功：{data_dict}")
        
        # 单个修改模式
        else:
            config[section][key] = str(value)
            print(f"修改配置成功：[{section}] {key} = {value}")

        # 写入配置文件（覆盖原文件，保留所有原有配置）
        with open(config_path, "w", encoding=encoding) as f:
            config.write(f)
        
        return True

    except Exception as e:
        print(f"修改配置文件失败：{e}")
        return False