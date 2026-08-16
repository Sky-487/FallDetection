import os
import numpy as np
import matplotlib.pyplot as plt
import pandas as pd

# 常量定义
SAMPLES_NUMBER = 100            # 每个动作的样本组数
SAMPLES_PER_GESTURE = 130       # 每个动作的总采样点数量
# MPU6050的数据计算
# 加速度：假设加速度量程范围设为+-8g，以X轴加速度为例，加速度原始读数Ax（16位ADC, 范围-2^15(对应-8g)~+2^15(对应+8g)）, 
# 实际X加速度为(2*Ax/2^16)*8g = 8g*Ax/2^15 = 8*Ax/32768 (g)
# 如下定义加速度量程范围系数AccRange_Coff，则实际X加速度为2*AccRange_Coff*g*Ax/2^16 = AccRange_Coff*Ax/2^15 (g) = AccRange_Coff*Ax/32768 (g)
# 角速度类似，假设角速度量程范围设为+-1000deg/s(即°/s)，以X轴角速度为例，角速度原始读数Gx（16位ADC, 范围-2^15(对应-1000deg/s)~+2^15(对应+1000deg/s)）, 
# 如下定义加速度量程范围系数GyroRange_Coff，则实际X加速度为2*GyroRange_Coff*Gx/2^16 = GyroRange_Coff*Gx/2^15 = GyroRange_Coff*Gx/32768 (deg/s)
# AccRange_Coff、GyroRange_Coff应根据ESP32端MPU6050初始化代码init()中的具体设置确定！
AccRange_Coff = 8        # 加速度量程范围系数：如加速度量程范围+-2g，则AccRange_Coff设为2；
                         # 其余同理 +-4g: 4;    +-8g: 8;    +-16g: 16
GyroRange_Coff = 1000    # 角速度量程范围系数：如角速度量程范围+-250deg/s(即°/s)，则GyroRange_Coff设为250；
                         # 其余同理 +-500deg/s: 500;    +-1000deg/s: 1000;    +-2000deg/s: 2000
# 传回的六轴数据范围为[-32768,+32768]；除以32768后为归一化的六轴数据范围为[-1,+1]用于训练，此处画曲线时应根据量程系数及单位还原。

SampleFrequency = 50     # 采样频率，单位Hz. 应与ESP32端设置相同


def load_and_process_data(file_path, sample_index):
    """加载并处理数据"""
    if sample_index < 1 or sample_index > SAMPLES_PER_GESTURE:
        raise ValueError(f"样本索引应在[1, {SAMPLES_PER_GESTURE}]范围内")
    
    # 读取CSV文件（制表符分隔）
    df = pd.read_csv(file_path, sep='\t', header=None, names=['Ax', 'Ay', 'Az', 'Gx', 'Gy', 'Gz'])
    
    # 计算起始和结束行索引
    start_idx = (sample_index - 1) * SAMPLES_PER_GESTURE
    end_idx = start_idx + SAMPLES_PER_GESTURE
    
    # 提取指定样本数据
    sample_data = df.iloc[start_idx:end_idx].copy()
    
    # 数据转换
    sample_data[['Ax', 'Ay', 'Az']] = sample_data[['Ax', 'Ay', 'Az']].astype(float) / 32768 * AccRange_Coff
    sample_data[['Gx', 'Gy', 'Gz']] = sample_data[['Gx', 'Gy', 'Gz']].astype(float) / 32768 * GyroRange_Coff
    
    # 计算合加速度
    sample_data['A_combined'] = np.sqrt(sample_data['Ax']**2 + sample_data['Ay']**2 + sample_data['Az']**2)
    
    # 创建时间轴
    time_axis = np.arange(SAMPLES_PER_GESTURE) / SampleFrequency      # 横轴为采样时间：默认SAMPLES_PER_GESTURE = 130，SampleFrequency = 50Hz，则范围为[0s,2.6s]
    # time_axis = np.arange(SAMPLES_PER_GESTURE)                          # 横轴为采样点计数，默认SAMPLES_PER_GESTURE = 130，则范围为[1,130]
                                                                        # 注意：np.arange(SAMPLES_PER_GESTURE) 按默认值实际范围为[0,129]
    # print(time_axis)                                                                    
    
    return sample_data, time_axis

def plot_sample_data(sample_data, time_axis, sample_index):
    """绘制样本数据图形"""
    # plt.figure(figsize=(15, 10))
    plt.figure(figsize=(12, 10))
    
    # 图1: 合加速度
    plt.subplot(3, 1, 1)
    plt.plot(time_axis, sample_data['A_combined'], 'm-', linewidth=2, label='Resultant Acceleration')
    plt.title(f'Sample {sample_index} - Resultant Acceleration')
    plt.xlabel('Time (s)')
    # plt.xlabel('Samples')
    plt.ylabel('Acceleration (g)')
    plt.legend()
    plt.grid(True)

    # 图2: 三轴加速度
    plt.subplot(3, 1, 2)
    plt.plot(time_axis, sample_data['Ax'], 'r-', label='Ax')
    plt.plot(time_axis, sample_data['Ay'], 'g-', label='Ay')
    plt.plot(time_axis, sample_data['Az'], 'b-', label='Az')
    plt.title(f'Sample {sample_index} - 3-axis Acceleration Data')
    plt.xlabel('Time (s)')
    # plt.xlabel('Samples')
    plt.ylabel('Acceleration (g)')
    plt.legend()
    plt.grid(True)    
    
    # 图3: 三轴角速度
    plt.subplot(3, 1, 3)
    plt.plot(time_axis, sample_data['Gx'], 'r-', label='Gx')
    plt.plot(time_axis, sample_data['Gy'], 'g-', label='Gy')
    plt.plot(time_axis, sample_data['Gz'], 'b-', label='Gz')
    plt.title(f'Sample {sample_index} - 3-axis Gyroscope Data')
    plt.xlabel('Time (s)')
    # plt.xlabel('Samples')
    plt.ylabel('Angular Velocity (°/s)')
    plt.legend()
    plt.grid(True)
    
    plt.tight_layout()
    plt.show()

def main():
    # 输入要显示的样本索引
    while True:
        try:
            i = int(input(f"请输入要显示的样本索引[1-{SAMPLES_NUMBER}] (输入0退出): "))
            if i == 0:
                print("程序退出")
                break
            if i < 1 or i > SAMPLES_NUMBER:
                print(f"输入无效，请输入1-{SAMPLES_NUMBER}之间的整数")
                continue          
            
            # 处理数据并绘图
            # file_path = os.path.join(os.path.dirname(__file__), 'data.csv')
            file_path = os.path.join(os.path.dirname(__file__), "data", "data.csv")
            # print(file_path) 
            # sample_data, time_axis = load_and_process_data('data.csv', i)
            sample_data, time_axis = load_and_process_data(file_path, i)
            plot_sample_data(sample_data, time_axis, i)
            
        except ValueError as e:
            print(f"错误: {e}")
        except FileNotFoundError:
            print("错误: 未找到data.csv文件，请确保它在当前目录下")
            break
        except Exception as e:
            print(f"发生未知错误: {e}")
            break

if __name__ == "__main__":
    main()