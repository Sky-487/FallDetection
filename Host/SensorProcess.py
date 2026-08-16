from asyncio.windows_events import NULL
import socket
from unittest import skip
from PerformanceCounter import PerformanceCounter
from SensorChart import SensorChart
from DataFile import DataFile

# PRESAMPLE_COUNT = 65        # 每个动作监测阈值前的采样点数 65, 50Hz采样时为1.3s秒
# POSTSAMPLE_COUNT = 65       # 每个动作监测阈值后的采样点数 65, 50Hz采样时约为1.3s秒
PRESAMPLE_COUNT = 50        # 每个动作监测阈值前的采样点数 50, 50Hz采样时为1.0s秒
POSTSAMPLE_COUNT = 80       # 每个动作监测阈值后的采样点数 80, 50Hz采样时约为1.6s秒
SAMPLES_PER_GESTURE = PRESAMPLE_COUNT + POSTSAMPLE_COUNT        # New: 每个动作总采样点数量

class UdpServer:
    def __init__(self):
        self.Sample = []
        self.Data = []
        self.Idle = True
        self.SampleCount = 0

        self.Counter = PerformanceCounter()
        self.Chart = SensorChart()
        self.DataFile = DataFile()

        self.Server = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
        self.Server.setsockopt(socket.SOL_SOCKET, socket.SO_BROADCAST, 1)
        self.Server.setsockopt(socket.SOL_SOCKET, socket.SO_RCVBUF, 65536 * 16)
        self.Server.settimeout(0.5)

        # GetLocalIP()内的地址根据本地IP的子网地址修改
        # ip = self.GetLocalIP("192.168.3")
        ip = self.GetLocalIP("192.168.224")
        # ip = self.GetLocalIP("192.168.43")
        print(ip)
        self.Server.bind((ip, 8000)) 


    def Run(self):
        self.Chart.Run(self.Recv) 
        self.DataFile = NULL


    def Recv(self):
        #self.Data = [0] * 9
        #New改: 每组IMU数据由原来的9个改为6个(注:每个数据含4 Bytes), 原最后3个实际没用
        self.Data = [0] * 6

        try:
            #data, address = self.Server.recvfrom(40)
            #New改: 每组IMU数据由原来的9个改为6个(注:每个数据含4 Bytes), 加上第一个为采样点计数值4 Bytes, 共计6*4+4=28 Bytes (原代码9*4+4=40)
            data, address = self.Server.recvfrom(28)

            #print(len(data), data)
            reader = lambda p: int.from_bytes(data[p:p + 4], byteorder="little", signed=True)

            #读回的第一个为MCU端采样点的当前计数值, 赋给c
            c = reader(0)    
            #for i in range(9):
            #New改: 每组IMU数据由原来的9个改为6个(注:每个数据含4 Bytes), 原最后3个实际没用
            for i in range(6):
               self.Data[i]  = reader((i + 1) * 4)
            
            if self.Idle: 
                self.Sample = []
                self.Idle = False

            while self.AddData(c, self.Data[:]) < c:
                continue 


        except socket.timeout:
            if not self.Idle:
                # if len(self.Sample) == 120:   # New: 每个动作120个采样点, 配合采样频率100Hz
                # if len(self.Sample) == 60:   # New: 每个动作60个采样点, 配合采样频率50Hz
                if len(self.Sample) == SAMPLES_PER_GESTURE:      # New: 每个动作总采样点数量SAMPLES_PER_GESTURE
                    self.SampleCount += 1
                    for d in self.Sample:
                        self.DataFile.Write(d)
                else:
                    self.Sample = []
                #样本接收成功的次数    
                print("Sample: ", self.SampleCount)

            self.DataFile.Close()
            self.Counter.Reset()
            self.Idle = True

        except Exception as e:
            self.Data = []
            print("error", e.args)


    def AddData(self, c, d):
            self.Sample.append(d)
            #打印信息依次为：count为接收一次网络UDP数据（即一个采样点）后PC端自己的计数值; t * 1000处理花费的时间ms; MCU端采样点的当前计数值; 一个采样点数据(3个加速度3个角速度)
            [count, t] = self.Counter.Frame()
            print(count, int(t * 1000), c, d)

            self.Chart.Draw(d, self.Sample)

            return count


    def GetLocalIP(self, mask):
        host_name = socket.gethostname()
        ips = socket.gethostbyname_ex(host_name)[2]

        for ip in ips:
            if ip.startswith(mask):
                ret = ip    

        return ret       



if __name__ == '__main__':
    Server = UdpServer()
    Server.Run()
