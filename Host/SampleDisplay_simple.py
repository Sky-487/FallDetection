import os
import pandas as pd
import matplotlib.pyplot as plt
import numpy as np

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

def plot_sample_data(i, samples_per_group=130):
    try:
        # 获取当前脚本所在目录
        script_dir = os.path.dirname(os.path.abspath(__file__))
        # csv_path = os.path.join(script_dir, 'data.csv')
        csv_path = os.path.join(script_dir, "data", 'data.csv')
        # print(file_path) 
        
        # 读取CSV文件，指定制表符分隔和没有表头
        df = pd.read_csv(csv_path, sep='\t', header=None)
        # df = pd.read_csv(csv_path, sep='\t', header=None, names=['Ax', 'Ay', 'Az', 'Gx', 'Gy', 'Gz'])
        
        # 检查数据是否完整
        total_samples = len(df)
        if total_samples < i * samples_per_group:
            print(f"错误：文件中的数据不足以包含第{i}组样本（需要{i*samples_per_group}行，实际只有{total_samples}行）")
            return
        
        # 提取第i组样本数据（组号从1开始）
        start_idx = (i - 1) * samples_per_group
        end_idx = i * samples_per_group
        group_data = df.iloc[start_idx:end_idx]

        # 默认横坐标按组递增连续计数，增加此句每组显示时横坐标按从1开始计数，范围为[1,130]
        sample_count = np.arange(len(group_data))               # 默认samples_per_group=130，则np.arange(len(group_data))范围为[0,129]
        # sample_count = np.arange(len(group_data)) + 1               # 默认samples_per_group=130，则np.arange(len(group_data))范围为[0,129]
        # 根据采样频率将横坐标转换为时间(秒)显示，范围为[0s,2.58s]
        time_display = np.arange(len(group_data)) / SampleFrequency  # 默认samples_per_group=130，SampleFrequency=50Hz
        
        # 检查数据列数
        if group_data.shape[1] < 6:
            print("错误：数据列数不足，每组样本应包含6列数据（Ax,Ay,Az,Gx,Gy,Gz）")
            return
        
        # 提取加速度和角速度数据
        # acc_data = group_data.iloc[:, 0:3].astype(float) * AccRange_Coff / 32768     # 前三列是加速度
        # gyro_data = group_data.iloc[:, 3:6].astype(float) * GyroRange_Coff / 32768   # 后三列是角速度
        acc_data = group_data.iloc[:, 0:3].astype(float) * AccRange_Coff / 32768     # 前三列是加速度
        gyro_data = group_data.iloc[:, 3:6].astype(float) * GyroRange_Coff / 32768   # 后三列是角速度

        # 计算合加速度（三轴加速度的平方根）
        resultant_acc = np.sqrt(acc_data.iloc[:, 0]**2 + acc_data.iloc[:, 1]**2 + acc_data.iloc[:, 2]**2) 
        
        # 创建图形
        # plt.figure(figsize=(12, 8))
        plt.figure(figsize=(12, 10))
        
        # 单独绘制合加速度曲线
        plt.subplot(3, 1, 1)
        # plt.plot(resultant_acc, 'm-', label='Resultant Acceleration', linewidth=2)                # 横坐标按组递增连续计数
        plt.plot(sample_count, resultant_acc, 'm-', label='Resultant Acceleration', linewidth=2)    # 横坐标按组从0开始计数
        plt.title(f'Resultant Acceleration curve - Sample Group No.{i}')
        # plt.xlabel('Sample Count')
        plt.xlabel('Time (s)')          # 横坐标为采样持续时间，范围为[0,(samples_per_group-1)/SampleFrequency]，默认[0s,2.58s]
        # plt.xticks(sample_count[::10], time_display[::10])  # 利用plt.xticks() 将横坐标轴刻度的位置和标签改为时间
        plt.xticks(sample_count[::5], time_display[::5])  # 利用plt.xticks() 将横坐标轴刻度的位置和标签改为时间
        plt.ylabel('Resultant Acceleration (g)')
        plt.legend()
        plt.grid(True)
       

        # 绘制加速度曲线
        # plt.subplot(2, 1, 1)
        plt.subplot(3, 1, 2)
         # plt.plot(acc_data.iloc[:, 0], 'r-', label='Ax')       # 横坐标按组递增连续计数
        # plt.plot(acc_data.iloc[:, 1], 'g-', label='Ay')
        # plt.plot(acc_data.iloc[:, 2], 'b-', label='Az')
        # plt.plot(resultant_acc, 'm--', label='Resultant Acceleration', linewidth=2)       # 同时绘制合加速度曲线
        plt.plot(sample_count, acc_data.iloc[:, 0], 'r-', label='Ax')     # 横坐标按组从0开始计数
        plt.plot(sample_count, acc_data.iloc[:, 1], 'g-', label='Ay')
        plt.plot(sample_count, acc_data.iloc[:, 2], 'b-', label='Az')
        # plt.plot(sample_count, resultant_acc, 'm--', label='Resultant Acceleration', linewidth=2)       # 同时绘制合加速度曲线
        plt.title(f'3-axis Accelerometer curve - Sample Group No.{i}')
        # plt.xlabel('Sample Count')    # 横坐标为采样点计数值，范围为[1,samples_per_group]，默认[1,130]。如改为时间，则注销该句，替换为下面两句
        plt.xlabel('Time (s)')          # 横坐标为采样持续时间，范围为[0,(samples_per_group-1)/SampleFrequency]，默认[0s,2.58s]
        # plt.xticks(sample_count[::10], time_display[::10])  # 利用plt.xticks() 将横坐标轴刻度的位置和标签改为时间
        plt.xticks(sample_count[::5], time_display[::5])  # 利用plt.xticks() 将横坐标轴刻度的位置和标签改为时间
        plt.ylabel('Acceleration (g)')
        plt.legend()        
        plt.grid(True)
        
        # 绘制角速度曲线
        # plt.subplot(2, 1, 2)
        plt.subplot(3, 1, 3)
        # plt.plot(gyro_data.iloc[:, 0], 'r-', label='Gx')              # 横坐标按组递增连续计数
        # plt.plot(gyro_data.iloc[:, 1], 'g-', label='Gy')
        # plt.plot(gyro_data.iloc[:, 2], 'b-', label='Gz')
        plt.plot(sample_count, gyro_data.iloc[:, 0], 'r-', label='Gx')  # 横坐标按组从0开始计数
        plt.plot(sample_count, gyro_data.iloc[:, 1], 'g-', label='Gy')
        plt.plot(sample_count, gyro_data.iloc[:, 2], 'b-', label='Gz')
        plt.title(f'3-axis Gyroscope curve - Sample Group No.{i}')
        # plt.xlabel('Sample Count')
        plt.xlabel('Time (s)')          # 横坐标为采样持续时间，范围为[0,(samples_per_group-1)/SampleFrequency]，默认[0s,2.58s]
        # plt.xticks(sample_count[::10], time_display[::10])  # 利用plt.xticks() 将横坐标轴刻度的位置和标签改为时间
        plt.xticks(sample_count[::5], time_display[::5])  # 利用plt.xticks() 将横坐标轴刻度的位置和标签改为时间
        plt.ylabel('Angular Speed (°/s)')
        plt.legend()
        plt.grid(True)
        
        plt.tight_layout()
        plt.show()
        
    except FileNotFoundError:
        print("错误：未找到data.csv文件，请确保它与脚本在同一目录下")
    except ValueError as e:
        print(f"数据格式错误：{str(e)}，请检查数据是否为有效的数字")
    except Exception as e:
        print(f"发生错误：{str(e)}")

# 示例：绘制第1组样本数据
plot_sample_data(1)