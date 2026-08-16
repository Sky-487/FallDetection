//BUTTOM_PIN改为BUTTOM_PIN，管脚位置修改，setup()中INPUT_PULLUP改为INPUT；WiFi接入SSID和密码修改
//UDPChannel.cpp的Send函数中增加目标IP和端口, 通过预定义值DEST_IP和DEST_PORT修改
#include <TensorFlowLite_ESP32.h>
#include <tensorflow/lite/experimental/micro/kernels/all_ops_resolver.h>
#include <tensorflow/lite/experimental/micro/micro_error_reporter.h>
#include <tensorflow/lite/experimental/micro/micro_interpreter.h>
#include <tensorflow/lite/schema/schema_generated.h>
#include <tensorflow/lite/version.h>

#include <Arduino.h>
#include <Esp.h>
#include <HardwareSerial.h>
#include <WiFi.h>
#include <WiFiUdp.h>

// I2Cdev and MPU6050 must be installed as libraries, or else the .cpp/.h files
// for both classes must be in the include path of your project
#include "I2Cdev.h"
#include "MPU6050.h"
// Arduino Wire library is required if I2Cdev I2CDEV_ARDUINO_WIRE implementation
// is used in I2Cdev.h
#if I2CDEV_IMPLEMENTATION == I2CDEV_ARDUINO_WIRE
    #include "Wire.h"
#endif

#include "UDPChannel.h"
#include "LED.h"

#include "model.h"

const int BUTTON_PIN = 13;  //GPIO13, =D7(FireBeetle), =D13(ESP32_30Pin)

// const char* AP_SSID = "HUAWEI-Hans";  //根据实际修改
const char* AP_SSID = "Magic7Pro-Hans";  //根据实际修改
// const char* AP_SSID = "HUAWEIP30Pro-Hans";  //根据实际修改
const char* AP_PWD = "Coco20090411";  //根据实际修改

// const char* DEST_IP = "192.168.3.255";  //新增加, 修改为对应PC端IP地址(如："192.168.3.10")或者所在子网的广播地址(如："192.168.3.255")
const char* DEST_IP = "192.168.224.203"; 
// const char* DEST_IP = "192.168.43.86"; 
unsigned int DEST_PORT = 8000;  //新增加, 修改为对应PC端接收端口号(SensorProcess.py中对应为8000)
unsigned int DEST_PORT2 = 8888;  //New add, 修改为对应PC端接收端口号(ResultDisplay.py中对应为8888)

// New add
#define WORKMODE            1   // 0: 训练模式TrainingMode(); 1: 识别模式PredictMode()

//#define FREQUENCY_HZ        100
#define FREQUENCY_HZ        50
#define INTERVAL_MS         (1000 / (FREQUENCY_HZ + 1))   // New改: replaced by mills(). 参考ESP32-TinyML-main和TinyML_GestureRecognition_Esp32-main, 频率转换为时间间隔时需要加1

#define P_Threshold         0.90  // 概率阈值, 当相应动作的输出概率大于此阈值时成立0.90

//const int SAMPLE_COUNT = 120;   // 每个动作120个采样点, 配合采样频率100Hz(#define FREQUENCY_HZ        100)
//const int SAMPLE_COUNT = 60;    // 每个动作60个采样点, 配合采样频率50Hz(#define FREQUENCY_HZ        50)
//跌倒检测
// #define PRESAMPLE_COUNT     65    // 每个动作监测阈值前的采样点数 65, 50Hz采样时为1.3s秒
// #define POSTSAMPLE_COUNT    65    // 每个动作监测阈值后的采样点数 65, 50Hz采样时约为1.3s秒
#define PRESAMPLE_COUNT     50    // 每个动作监测阈值前的采样点数 50, 50Hz采样时为1.0s秒
#define POSTSAMPLE_COUNT    80    // 每个动作监测阈值后的采样点数 80, 50Hz采样时约为1.6s秒(1.58秒)
//手势识别
// #define PRESAMPLE_COUNT     15    // 每个动作监测阈值前的采样点数 15
// #define POSTSAMPLE_COUNT    65    // 每个动作监测阈值后的采样点数 65
#define SAMPLE_COUNT        (PRESAMPLE_COUNT + POSTSAMPLE_COUNT)      // 每个动作的采样点总数

#define ACCELERATION_THRESHOLD    1.8   // 如ACCELERATION_THRESHOLD = 2.5, 表示2.5g, 注意不动时Z轴=1g, 故平时不动的三轴加速度均方根即有1g
                                        // 2.5：不容易触发，正常走路一般不会触发，坐着起立一般不会触发；大步快速走、狠狠坐下一般可触发。
                                        // 2.0(预测模式如1.8时触发频繁，也可用此)：不太容易触发，正常走路、坐着起立、坐下不易触发；大步快速走、较重坐着起立、较重坐下一般可触发。
                                        // 1.8(预测模式如1.6时触发频繁，也可用此)：较容易触发，正常走路、稍快走路、轻轻坐着起立、轻轻坐下不易触发；快走停下、稍重坐着起立、稍重坐下、上下楼梯一般可以触发；
                                        // 1.7：较容易触发，正常走路、轻轻坐着起立、轻轻坐下不易触发；稍快走路、稍重坐着起立、稍重坐下、上下楼梯容易触发；
                                        // 1.6(训练模式采用，预测模式也可用)：容易触发，正常走路、坐着起立、坐下都较容易触发；
//const int SMOTH_COUNT = 5;
#define SMOTH_COUNT         1       // New改: Acquisition()根据该参数确定每次采样连续读几次MPU6050, 且将这些读数全部相加后作为输出, 相当于平滑且扩大了SMOTH_COUNT倍, 原代码为5

// const char* GESTURES[] = {  
//   "fall",
//   "jump",
//   "jog",
//   "walk"
// };
const char* GESTURES[] = {  
  "fall",
  "jump",
  "jog",
  "sitdown"
};
// const char* GESTURES[] = {  
//   "fall",
//   "jump",
//   "jog",
//   "walk",
//   "sitdown",
//   "downstairs"
// };
// const char* GESTURES[] = {  
//   "fall",
//   "jump",
//   "jog",
//   "walk",
//   "sitdown",
//   "situp",
//   "downstairs",
//   "upstairs"
// };

#define NUM_GESTURES    (sizeof(GESTURES) / sizeof(GESTURES[0]))

static unsigned long last_interval_ms = 0;

float AccRange_Coff = 2;      // New: 根据加速度量程范围确定系数, 2g: 2; 4g: 4; 8g: 8; 16g: 16. 此处预定义2只是默认值, setup()中会根据量程返回重新定义
                              //       用于计算不同量程下的加速度值(g): 如量程为AccRange_Coff*g, x轴加速度原始读数ax(16位ADC, 范围-2^15(对应-4g)~+2^15(对应+4g)), 
                              //       实际X加速度为2*AccRange_Coff*g*ax/2^16 = AccRange_Coff*ax/2^15 (g) = AccRange_Coff*ax/32768 (g)

unsigned int RecordCount = 0;   // 采样计数器
unsigned int SendCount = 0;     // 向上位机发送计数器

#define PRE_SAMPLE        0
#define CHK_THRESHOLD     1
#define POST_SAMPLE       2

unsigned int SampleState = 0;
// PRE_SAMPLE: 表示数据预采集未完成(RecordCount <= PRESAMPLE_COUNT - 1),  新采样一个数据进来后, 根据当前RecordCount计数值, 数据根据采集顺序依次存入
//    [0], [1], ..., [RecordCount], 每采集一个样点SampleState不变, RecordCount加1, 直到RecordCount = PRESAMPLE_COUNT - 1时SampleState = CHK_THRESHOLD. 
// CHK_THRESHOLD: 表示数据预采集完成(RecordCount = PRESAMPLE_COUNT), 此时根据最新采样点进行阈值检测:
//    (1)若判断未超过阈值则: SampleState和RecordCount保持不变, 且待新采样一个数据进来后, 最早的一个数据[0]丢弃,  
//       其余数据依次左移[0]<-[1], ..., [PRESAMPLE_COUNT-2]<-[PRESAMPLE_COUNT-1], [PRESAMPLE_COUNT-1]<-[新采数据]
//    (2)若判断超过阈值则: SampleState = POST_SAMPLE, RecordCount继续加1, 前面数据[0], [1], ..., [PRESAMPLE_COUNT - 1]不动, [PRESAMPLE_COUNT]<-[新采数据]
// POST_SAMPLE: 表示触发条件满足, 进行后段数据采集(PRESAMPLE_COUNT + 1 =< RecordCount <= PRESAMPLE_COUNT + POSTSAMPLE_COUNT - 1). 新采样一个数据进来后, 根据当前 
//    RecordCount计数值, 按采集顺序数据依次存入[PRESAMPLE_COUNT], [PRESAMPLE_COUNT + 1], ..., [PRESAMPLE_COUNT + POSTSAMPLE_COUNT - 1].
//    RecordCount继续加1, SampleState不变, 直到RecordCount = PRESAMPLE_COUNT + POSTSAMPLE_COUNT - 1时:
//    (1)如是训练模式TrainingMode(), 在上述存储采样点的同时将采样点从[0]开始逐一传给上位机, 当采样计数RecordCount = PRESAMPLE_COUNT + POSTSAMPLE_COUNT - 1时.
//       后段数据全部POSTSAMPLE_COUNT个已采集存储完成, 而此时向上位机总共传送了POSTSAMPLE_COUNT-1个采样点, 仍需继续上传剩余PRESAMPLE_COUNT+1个采样点.
//       完成后恢复初始, 即RecordCount = 0, SampleState = PRE_SAMPLE.
//    (2)如是识别模式PredictMode(), 当采样计数RecordCount = PRESAMPLE_COUNT + POSTSAMPLE_COUNT - 1时执行Inference()及Result(), 
//       完成后恢复初始, 即RecordCount = 0, SampleState = PRE_SAMPLE.

int16_t SampleData[6];    // 一组6个数据, 前3个加速度, 后3个角速度(陀螺仪)
int IMUData[6];
int SendData[6];
float Samples[6][SAMPLE_COUNT];     // 一组6个数据, 前3个加速度, 后3个角速度(陀螺仪), 每个动作监测阈值前后分别为PRESAMPLE_COUNT个和POSTSAMPLE_COUNT个采样点
                                    // 共计SAMPLE_COUNT = PRESAMPLE_COUNT + POSTSAMPLE_COUNT个.

char SerialBuffer[50]; 
// char ResultBuffer[4][20];   // New add for ResultDisplay.py: 4 output
// char ResultBuffer[6][20];   // New add for ResultDisplay.py: 6 output
char ResultBuffer[NUM_GESTURES][20];   // New add for ResultDisplay.py: 动作个数NUM_GESTURES上面计算获得，每个动作最多20个char

UDPChannel Channel;           

tflite::MicroErrorReporter tflErrorReporter;
tflite::ops::micro::AllOpsResolver tflOpsResolver;

const tflite::Model* tflModel = nullptr;
tflite::MicroInterpreter* tflInterpreter = nullptr;
TfLiteTensor* tflInputTensor = nullptr;
TfLiteTensor* tflOutputTensor = nullptr;

constexpr int tensorArenaSize = 8 * 1024;
byte tensorArena[tensorArenaSize];

// class default I2C address is 0x68
// specific I2C addresses may be passed as a parameter here
// AD0 low = 0x68 (default for InvenSense evaluation board)
// AD0 high = 0x69
MPU6050 accelgyro;
//MPU6050 accelgyro(0x69); // <-- use for AD0 high

int16_t ax, ay, az;
int16_t gx, gy, gz;



void setup() {
  pinMode(BUTTON_PIN, INPUT_PULLUP);      //未外接上拉电阻
  //pinMode(BUTTON_PIN, INPUT);           //已外接上拉电阻，按钮按下时为低

  // pinMode(LED_PIN, OUTPUT);
  // pinMode(GREEN_PIN, OUTPUT);
  // pinMode(RED_PIN, OUTPUT);
  // pinMode(YELLOW_PIN, OUTPUT);

  // Led.Off();      //turn off LED BLUE
  // Led.Blank();    //turn off LED YELLOW&RED&GREEN
  
  Led.On();       //turn on LED BLUE
  delay(1000);
  Led.Yellow();   //turn on LED YELLOW
  delay(1000);
  Led.Red();      //turn on LED RED
  delay(1000);
  Led.Green();    //turn on LED GREEN
  delay(1000);

  Led.Off();      //turn off LED BLUE
  Led.Blank();    //turn off LED YELLOW&RED&GREEN

  // join I2C bus (I2Cdev library doesn't do this automatically)
  #if I2CDEV_IMPLEMENTATION == I2CDEV_ARDUINO_WIRE
      Wire.begin();
  #elif I2CDEV_IMPLEMENTATION == I2CDEV_BUILTIN_FASTWIRE
      Fastwire::setup(400, true);
  #endif

  Serial.begin(115200);
  Channel.ConnectAP(AP_SSID, AP_PWD);

  // 部分重要参数需在init()函数中修改!
  // (1)设置MPU6050的各轴偏置补偿, 需要使用MPU6050_Zero.ino获得具体偏置值(注意模块水平放置并上电预热时间5-10分钟, 具体见: 运行IMU_Zero获得最佳偏置.txt);
  // (2)修改setFullScaleGyroRange(), setFullScaleAccelRange(), setDLPFMode()的设置, 
  //  	默认分别为MPU6050_IMU::MPU6050_GYRO_FS_500, MPU6050_IMU::MPU6050_ACCEL_FS_4, MPU6050_IMU::MPU6050_DLPF_BW_42, 即+/-4g, +/-500degrees/sec, 数字低通滤波器带宽为42Hz
  init(); 

  if (!WORKMODE) {
    Serial.println("Training Mode");
  } 
  else {
    Serial.println("Predict Mode");
    LoadModel();
  }  
}

void loop() {
  if (millis() > last_interval_ms + INTERVAL_MS) {    // New change，replaced by mills()
    last_interval_ms = millis();
    //Serial.println(last_interval_ms);

    if (!WORKMODE) {
      TrainingMode();    //训练模式选择此
    } 
    else {
      PredictMode();        //识别模式选择此
    }  
  }
}

//
// Functions
//

void init() {
//initialize device
  Serial.println("Initializing I2C devices...");
//根据MPU6050.cpp的initialize()
 /** Power on and prepare for general usage.
 * This will activate the device and take it out of sleep mode (which must be done
 * after start-up). This function also sets both the accelerometer and the gyroscope
 * to their most sensitive settings, namely +/- 2g and +/- 250 degrees/sec, and sets
 * the clock source to use the X Gyro for reference, which is slightly better than
 * the default internal clock source.
 */
  accelgyro.initialize();     // 原始代码默认值为+/- 2g and +/- 250 degrees/sec
  //verify connection
  Serial.println("Testing device connections...");
  Serial.println(accelgyro.testConnection() ? "MPU6050 connection successful" : "MPU6050 connection failed");

  /* Use the code below to change accel/gyro offset values. Use MPU6050_Zero.ino to obtain the recommended offsets */ 
  // 注意增加偏置修正后,最好样本重新获取训练! 目前初步测试影响不大.
  Serial.println("Updating internal sensor offsets...\n");
  //FireBeetle Board-ESP32跌倒检测板
  // accelgyro.setXAccelOffset(-901); //Set your accelerometer offset for axis X
  // accelgyro.setYAccelOffset(-3929); //Set your accelerometer offset for axis Y
  // accelgyro.setZAccelOffset(1704); //Set your accelerometer offset for axis Z
  // accelgyro.setXGyroOffset(-204);  //Set your gyro offset for axis X
  // accelgyro.setYGyroOffset(51);  //Set your gyro offset for axis Y
  // accelgyro.setZGyroOffset(-22);  //Set your gyro offset for axis Z
  //FireBeetle跌倒检测白色盒子
  accelgyro.setXAccelOffset(1275); //Set your accelerometer offset for axis X
  accelgyro.setYAccelOffset(-3650); //Set your accelerometer offset for axis Y
  accelgyro.setZAccelOffset(1221); //Set your accelerometer offset for axis Z
  accelgyro.setXGyroOffset(-19);  //Set your gyro offset for axis X
  accelgyro.setYGyroOffset(-28);  //Set your gyro offset for axis Y
  accelgyro.setZGyroOffset(-14);  //Set your gyro offset for axis Z
  /*Print the defined offsets*/
  Serial.print("\t");
  Serial.print("AccXOffset");
  Serial.print("\t");
  Serial.print("AccYOffset"); 
  Serial.print("\t");
  Serial.print("AccZOffset");
  Serial.print("\t");
  Serial.print("GyroXOffset"); 
  Serial.print("\t");
  Serial.print("GyroYOffset");
  Serial.print("\t");
  Serial.print("GyroZOffset");
  Serial.print("\n");

  Serial.print("\t");
  Serial.print(accelgyro.getXAccelOffset());
  Serial.print("\t");
  Serial.print("\t");
  Serial.print(accelgyro.getYAccelOffset()); 
  Serial.print("\t");
  Serial.print("\t");
  Serial.print(accelgyro.getZAccelOffset());
  Serial.print("\t");
  Serial.print("\t");
  Serial.print(accelgyro.getXGyroOffset()); 
  Serial.print("\t");
  Serial.print("\t");
  Serial.print(accelgyro.getYGyroOffset());
  Serial.print("\t");
  Serial.print("\t");
  Serial.print(accelgyro.getZGyroOffset());
  Serial.print("\n");
  Serial.println();

  accelgyro.setFullScaleGyroRange(MPU6050_IMU::MPU6050_GYRO_FS_1000);//	New 测试由默认值改为+/- 4g and +/- 500 degrees/sec
  Serial.print("Gyro range set to: ");
  switch (accelgyro.getFullScaleGyroRange()) {
    case MPU6050_IMU::MPU6050_GYRO_FS_250:
      Serial.println("+- 250 deg/s");
      break;
    case MPU6050_IMU::MPU6050_GYRO_FS_500:
      Serial.println("+- 500 deg/s");
      break;
    case MPU6050_IMU::MPU6050_GYRO_FS_1000:
      Serial.println("+- 1000 deg/s");
      break;
    case MPU6050_IMU::MPU6050_GYRO_FS_2000:
      Serial.println("+- 2000 deg/s");
      break;
  }

  accelgyro.setFullScaleAccelRange(MPU6050_IMU::MPU6050_ACCEL_FS_8);
  Serial.print("Accelerometer range set to: ");
  switch (accelgyro.getFullScaleAccelRange()) {
    case MPU6050_IMU::MPU6050_ACCEL_FS_2:
      Serial.println("+-2g");
      AccRange_Coff = 2;
      break;
    case MPU6050_IMU::MPU6050_ACCEL_FS_4:
      Serial.println("+-4g");
      AccRange_Coff = 4;
      break;
    case MPU6050_IMU::MPU6050_ACCEL_FS_8:
      Serial.println("+-8g");
      AccRange_Coff = 8;
      break;
    case MPU6050_IMU::MPU6050_ACCEL_FS_16:
      Serial.println("+-16g");
      AccRange_Coff = 16;
      break;
  }
  
  bool Rate_flag = 0;   //用于根据数字低通滤波器设置来区分内部输出速率: 0: 1kHz, 1: 8kHz
  accelgyro.setDLPFMode(MPU6050_IMU::MPU6050_DLPF_BW_42);   // New 数字低通滤波器带宽为42Hz，MPU6050输出速率为1kHz. 根据MPU6050.cpp的setDLPFMode(uint8_t mode)，参考getDLPFMode()的值说明设置
  Serial.print("Filter bandwidth set to: ");
  switch (accelgyro.getDLPFMode()) {
    case MPU6050_IMU::MPU6050_DLPF_BW_256:
      Serial.println("256 Hz"); 
      Rate_flag = 1;     
      break;
    case MPU6050_IMU::MPU6050_DLPF_BW_188:
      Serial.println("188 Hz");
      break;
    case MPU6050_IMU::MPU6050_DLPF_BW_98:
      Serial.println("98 Hz");
      break;
    case MPU6050_IMU::MPU6050_DLPF_BW_42:
      Serial.println("42 Hz");
      break;
    case MPU6050_IMU::MPU6050_DLPF_BW_20:
      Serial.println("20 Hz");
      break;
    case MPU6050_IMU::MPU6050_DLPF_BW_10:
      Serial.println("10 Hz");
      break;
    case MPU6050_IMU::MPU6050_DLPF_BW_5:
      Serial.println("5 Hz");
      break;
  } 

  //accelgyro.setRate(9);   // New：采样率设置为100Hz，DLPF滤波器截止频率一般设置为采样率的一半，根据上面介绍采样率和输出频率关系，SMPLRT_DIV=1000/100-1=9
  // New：根据现有的几个手势识别例子, MPU6050内部的采样率通常都不需要设置, 而是通过微处理器读数的时间间隔进行控制
  // 实际测试改参数设或不设置无区别! 读取MPU6050数据的函数Acquisition()中并未通过FIFO获取(getFIFOByte()。。。), 而采用getMotion6()函数获取数据, 此函数应该是根据最新的输出数据得到, 理论上只要内部输出速率
  // 大于MCU从传感器读取的频率, MCU得到的数据就不会有重复, 自动实现了降采样!
  Serial.print("MPU6050 Internal Sample Rate is: ");
  unsigned int Output_Rate = 1000;
  if (Rate_flag){
    Output_Rate = 8000;
  } else {
    Output_Rate = 1000;
  }
  Serial.print(Output_Rate / (1 + accelgyro.getRate()));
  Serial.println("Hz");
  
  Serial.print("MCU Sample Rate set to: ");
  Serial.print(FREQUENCY_HZ);
  Serial.println("Hz");

  Serial.print("Pre-Sample Numbers: ");
  Serial.print(PRESAMPLE_COUNT);
  Serial.print(", \t");
  Serial.print("Pre-Sample time: ");
  Serial.print((PRESAMPLE_COUNT * 1000.0) / FREQUENCY_HZ);
  Serial.println("ms");

  Serial.print("Post-Sample Numbers: ");
  Serial.print(POSTSAMPLE_COUNT);
  Serial.print(", \t");
  Serial.print("Post-Sample time: ");
  Serial.print((POSTSAMPLE_COUNT - 1) * 1000.0 / FREQUENCY_HZ);
  Serial.println("ms");
}


void LoadModel() {
  Serial.println("Loading Model");
  tflModel = tflite::GetModel(model);

  int ModelVersion = tflModel->version();

  sprintf(SerialBuffer, "Model Version: %d, TFLite Version: %d", ModelVersion, TFLITE_SCHEMA_VERSION);
  Serial.println(SerialBuffer);

  if (ModelVersion != TFLITE_SCHEMA_VERSION) {
    Serial.println("Model schema mismatch!");
  } else {
    Serial.println("Model Loaded!");
  }

  tflInterpreter = new tflite::MicroInterpreter(tflModel, tflOpsResolver, tensorArena, tensorArenaSize, &tflErrorReporter);
  tflInterpreter->AllocateTensors();
  tflInputTensor = tflInterpreter->input(0);
  tflOutputTensor = tflInterpreter->output(0);

  sprintf(SerialBuffer, "Input Size: %d[%d, %d], Outpu Size: %d[%d, %d]", tflInputTensor->dims->size, tflInputTensor->dims->data[0], tflInputTensor->dims->data[1], tflOutputTensor->dims->size, tflOutputTensor->dims->data[0], tflOutputTensor->dims->data[1]);
  Serial.println(SerialBuffer);
}


void TrainingMode() {
  Acquisition();
  // PRE_SAMPLE: 表示数据预采集未完成(RecordCount <= PRESAMPLE_COUNT - 1),  新采样一个数据进来后, 根据当前RecordCount计数值, 数据根据采集顺序依次存入
  //    [0], [1], ..., [RecordCount], 每采集一个样点SampleState不变, RecordCount加1, 直到RecordCount = PRESAMPLE_COUNT - 1时SampleState = CHK_THRESHOLD. 
  // CHK_THRESHOLD: 表示数据预采集完成(RecordCount = PRESAMPLE_COUNT), 此时根据最新采样点进行阈值检测:
  //    (1)若判断未超过阈值则: SampleState和RecordCount保持不变, 且待新采样一个数据进来后, 最早的一个数据[0]丢弃,  
  //       其余数据依次左移[0]<-[1], ..., [PRESAMPLE_COUNT-2]<-[PRESAMPLE_COUNT-1], [PRESAMPLE_COUNT-1]<-[新采数据]
  //    (2)若判断超过阈值则: SampleState = POST_SAMPLE, RecordCount继续加1, 前面数据[0], [1], ..., [PRESAMPLE_COUNT - 1]不动, [PRESAMPLE_COUNT]<-[新采数据]
  // POST_SAMPLE: 表示触发条件满足, 进行后段数据采集(PRESAMPLE_COUNT + 1 =< RecordCount <= PRESAMPLE_COUNT + POSTSAMPLE_COUNT - 1). 新采样一个数据进来后, 根据当前 
  //    RecordCount计数值, 按采集顺序数据依次存入[PRESAMPLE_COUNT], [PRESAMPLE_COUNT + 1], ..., [PRESAMPLE_COUNT + POSTSAMPLE_COUNT - 1].
  //    RecordCount继续加1, SampleState不变, 直到RecordCount = PRESAMPLE_COUNT + POSTSAMPLE_COUNT - 1时:
  //    (1)如是训练模式TrainingMode(), 在上述存储采样点的同时将采样点从[0]开始逐一传给上位机, 当采样计数RecordCount = PRESAMPLE_COUNT + POSTSAMPLE_COUNT - 1时.
  //       后段数据全部POSTSAMPLE_COUNT个已采集存储完成, 而此时向上位机总共传送了POSTSAMPLE_COUNT-1个采样点, 仍需继续上传剩余PRESAMPLE_COUNT+1个采样点.
  //       完成后恢复初始, 即RecordCount = 0, SampleState = PRE_SAMPLE.
  //    (2)如是识别模式PredictMode(), 当采样计数RecordCount = PRESAMPLE_COUNT + POSTSAMPLE_COUNT - 1时执行Inference()及Result(), 
  //       完成后恢复初始, 即RecordCount = 0, SampleState = PRE_SAMPLE.
  switch (SampleState) {
    case PRE_SAMPLE:
      for (int i = 0; i < 6; i++) {                   // 一组6个数据, 前3个加速度, 后3个角速度(陀螺仪). PRE_SAMPLE阶段共存了PRESAMPLE_COUNT个样点
        Samples[i][RecordCount] = IMUData[i];	        // 训练数据为裸数据, 不做归一化, 在PC端tinyml.py中除以32768.0, 做归一化处理!
      }
      if (RecordCount == PRESAMPLE_COUNT - 1) {
        SampleState = CHK_THRESHOLD;
      } 

      // if (RecordCount == 0) {
      //   Serial.print("SampleState = ");
      //   Serial.println(SampleState);
      // }
      // Serial.print("RecordCount = ");
      // Serial.println(RecordCount);

      RecordCount++;             
      break; 

    case CHK_THRESHOLD:
      if(DetectMotion() || digitalRead(BUTTON_PIN) == LOW) {
        //Serial.println();
        Serial.println("Blue Led On, Post-Sample & WiFi Transmission start!");   
        Led.Blank();    //turn off LED RED&GREEN
        Led.On();
        for (int i = 0; i < 6; i++) {                       // 符合触发条件, 则数据存入作为POST_SAMPLE阶段的第一个样点
          Samples[i][RecordCount] = IMUData[i];
        }

        // Serial.print("SampleState = ");
        // Serial.println(SampleState);
        // Serial.print("RecordCount = ");
        // Serial.println(RecordCount);

        SampleState = POST_SAMPLE;
        RecordCount++; 
        SendCount = 0;     
      }
      else{
        // 前(PRESAMPLE_COUNT-1)个采样点(每个样点6个数据)依次左移
        for (int k = 0; k < PRESAMPLE_COUNT - 1; k++) {
          for (int i = 0; i < 6; i++) {
            Samples[i][k] = Samples[i][k + 1];
          }

          // Serial.print("k = ");
          // Serial.println(k);
        }
        // 新采数据移入[PRESAMPLE_COUNT - 1]
        for (int i = 0; i < 6; i++) {
          Samples[i][PRESAMPLE_COUNT - 1] = IMUData[i];
        }
        
        // Serial.print("SampleState = ");
        // Serial.println(SampleState);
        // Serial.print("RecordCount = ");
        // Serial.println(RecordCount);
      }             
      break;

    case POST_SAMPLE:
      if(RecordCount <= SAMPLE_COUNT - 1) {   // 从PRESAMPLE_COUNT+1开始到SAMPLE_COUNT - 1, 共存了POSTSAMPLE_COUNT - 1个样点
        for (int i = 0; i < 6; i++) {
          Samples[i][RecordCount] = IMUData[i];
        } 

        // if(RecordCount == PRESAMPLE_COUNT + 1) {
        //   Serial.print("SampleState = ");
        //   Serial.println(SampleState);
        // }         
        // Serial.print("RecordCount = ");
        // Serial.println(RecordCount);  

        RecordCount++;
        // 向上位机传送采样点数据, 在采存后段样点阶段共发送POSTSAMPLE_COUNT - 1个(PRESAMPLE_COUNT + 1 =< RecordCount <= SAMPLE_COUNT - 1)
        for (int i = 0; i < 6; i++) {
          SendData[i] = Samples[i][SendCount];    // 从Samples[i][0]开始读出采样点以便传输
        } 
        SendCount++;    // 发送计数从1开始.
        Channel.Send(DEST_IP, DEST_PORT, SendCount , (unsigned char*)SendData, 24);   //每个采样点数据有6个轴, 每个轴数据为4byte, 共24 bytes 

        // Serial.print("1-SendCount = ");
        // Serial.println(SendCount);   
      }
      
      if(RecordCount == SAMPLE_COUNT) {
        //继续向上位机传送剩余PRESAMPLE_COUNT+1个采样点数据, (POSTSAMPLE_COUNT =< SendCount <= SAMPLE_COUNT)
        if(SendCount <= SAMPLE_COUNT - 1) {
          for (int i = 0; i < 6; i++) {
            SendData[i] = Samples[i][SendCount];    // 读出采样点以便传输
          }           
          SendCount++;  
          Channel.Send(DEST_IP, DEST_PORT, SendCount , (unsigned char*)SendData, 24);   //每个采样点数据有6个轴, 每个轴数据为4byte, 共24 bytes

          // Serial.print("2-SendCount = ");
          // Serial.println(SendCount);
        }
        else{
          // Serial.print("RecordCount = ");
          // Serial.println(RecordCount);
          // Serial.print("SendCount = ");
          // Serial.println(SendCount);

          Serial.println("Blue Led Off, WiFi Transmission stop!");    //New add
          Led.Off();
          SendCount = 0;
          RecordCount = 0;
          SampleState = PRE_SAMPLE;          
        }
      }         
      break;
  }
}


void PredictMode() {
  Acquisition();
  // PRE_SAMPLE: 表示数据预采集未完成(RecordCount <= PRESAMPLE_COUNT - 1),  新采样一个数据进来后, 根据当前RecordCount计数值, 数据根据采集顺序依次存入
  //    [0], [1], ..., [RecordCount], 每采集一个样点SampleState不变, RecordCount加1, 直到RecordCount = PRESAMPLE_COUNT - 1时SampleState = CHK_THRESHOLD. 
  // CHK_THRESHOLD: 表示数据预采集完成(RecordCount = PRESAMPLE_COUNT), 此时根据最新采样点进行阈值检测:
  //    (1)若判断未超过阈值则: SampleState和RecordCount保持不变, 且待新采样一个数据进来后, 最早的一个数据[0]丢弃,  
  //       其余数据依次左移[0]<-[1], ..., [PRESAMPLE_COUNT-2]<-[PRESAMPLE_COUNT-1], [PRESAMPLE_COUNT-1]<-[新采数据]
  //    (2)若判断超过阈值则: SampleState = POST_SAMPLE, RecordCount继续加1, 前面数据[0], [1], ..., [PRESAMPLE_COUNT - 1]不动, [PRESAMPLE_COUNT]<-[新采数据]
  // POST_SAMPLE: 表示触发条件满足, 进行后段数据采集(PRESAMPLE_COUNT + 1 =< RecordCount <= PRESAMPLE_COUNT + POSTSAMPLE_COUNT - 1). 新采样一个数据进来后, 根据当前 
  //    RecordCount计数值, 按采集顺序数据依次存入[PRESAMPLE_COUNT], [PRESAMPLE_COUNT + 1], ..., [PRESAMPLE_COUNT + POSTSAMPLE_COUNT - 1].
  //    RecordCount继续加1, SampleState不变, 直到RecordCount = PRESAMPLE_COUNT + POSTSAMPLE_COUNT - 1时:
  //    (1)如是训练模式TrainingMode(), 在上述存储采样点的同时将采样点从[0]开始逐一传给上位机, 当采样计数RecordCount = PRESAMPLE_COUNT + POSTSAMPLE_COUNT - 1时.
  //       后段数据全部POSTSAMPLE_COUNT个已采集存储完成, 而此时向上位机总共传送了POSTSAMPLE_COUNT-1个采样点, 仍需继续上传剩余PRESAMPLE_COUNT+1个采样点.
  //       完成后恢复初始, 即RecordCount = 0, SampleState = PRE_SAMPLE.
  //    (2)如是识别模式PredictMode(), 当采样计数RecordCount = PRESAMPLE_COUNT + POSTSAMPLE_COUNT - 1时执行Inference()及Result(), 
  //       完成后恢复初始, 即RecordCount = 0, SampleState = PRE_SAMPLE.
  switch (SampleState) {
    case PRE_SAMPLE:
      for (int i = 0; i < 6; i++) {                       // 一组6个数据, 前3个加速度, 后3个角速度(陀螺仪). PRE_SAMPLE阶段共存了PRESAMPLE_COUNT个样点
        Samples[i][RecordCount] = IMUData[i] / 32768.0;   // 归一化, 进入运算的数据范围变为[-1,1], 在PC端tinyml.py中同样做归一化处理!
      }
      if (RecordCount == PRESAMPLE_COUNT - 1) {
        SampleState = CHK_THRESHOLD;
      } 

      // if (RecordCount == 0) {
      //   Serial.print("SampleState = ");
      //   Serial.println(SampleState);
      // }
      // Serial.print("RecordCount = ");
      // Serial.println(RecordCount);

      RecordCount++;             
      break; 

    case CHK_THRESHOLD:
      if(DetectMotion() || digitalRead(BUTTON_PIN) == LOW) {
        //Serial.println();
        Serial.println("Blue Led On, Post-Sample start!");   
        Led.Blank();    //turn off LED RED&GREEN
        Led.On();
        for (int i = 0; i < 6; i++) {                       // 符合触发条件, 则数据存入作为POST_SAMPLE阶段的第一个样点
          Samples[i][RecordCount] = IMUData[i] / 32768.0;   // 归一化, 进入运算的数据范围变为[-1,1], 在PC端tinyml.py中同样做归一化处理!
        }

        // Serial.print("SampleState = ");
        // Serial.println(SampleState);
        // Serial.print("RecordCount = ");
        // Serial.println(RecordCount);

        SampleState = POST_SAMPLE;
        RecordCount++; 
        SendCount = 0;     
      }
      else{
        // 前(PRESAMPLE_COUNT-1)个采样点(每个样点6个数据)依次左移
        for (int k = 0; k < PRESAMPLE_COUNT - 1; k++) {
          for (int i = 0; i < 6; i++) {
            Samples[i][k] = Samples[i][k + 1];
          }

          // Serial.print("k = ");
          // Serial.println(k);
        }
        // 新采数据移入[PRESAMPLE_COUNT - 1]
        for (int i = 0; i < 6; i++) {
          Samples[i][PRESAMPLE_COUNT - 1] = IMUData[i] / 32768.0;   // 归一化, 进入运算的数据范围变为[-1,1], 在PC端tinyml.py中同样做归一化处理!
        }
        
        // Serial.print("SampleState = ");
        // Serial.println(SampleState);
        // Serial.print("RecordCount = ");
        // Serial.println(RecordCount);
      }             
      break;

    case POST_SAMPLE:
      if(RecordCount <= SAMPLE_COUNT - 1) {   // 从PRESAMPLE_COUNT+1开始到SAMPLE_COUNT - 1, 共存了POSTSAMPLE_COUNT - 1个样点
        for (int i = 0; i < 6; i++) {
          Samples[i][RecordCount] = IMUData[i] / 32768.0;   // 归一化, 进入运算的数据范围变为[-1,1], 在PC端tinyml.py中同样做归一化处理!
        } 

        // if(RecordCount == PRESAMPLE_COUNT + 1) {
        //   Serial.print("SampleState = ");
        //   Serial.println(SampleState);
        // }         
        // Serial.print("RecordCount = ");
        // Serial.println(RecordCount);  

        RecordCount++;   
      }
      
      if(RecordCount == SAMPLE_COUNT) {
        // Serial.print("RecordCount = ");
        // Serial.println(RecordCount);

        Serial.println("Blue Led Off, Post-Sample Post-Sample stop!");    //New add
        Led.Off();

        if (Inference()) Result();

        RecordCount = 0;
        SampleState = PRE_SAMPLE;
      }         
      break;
  }
}


bool DetectMotion() {
  float square_sum = 0;
  bool ret = 0;

  // 求传感器的前3个数据即三轴加速度ax,ay,az的平方和
  for (int i = 0; i < 3; i++) {
    float a = (float)IMUData[i];
    square_sum += a * a;
  }
  // 将三轴加速度平方和开根号(也即求出三轴加速度的均方根Root Mean Square, 或称矢量和Sum Vector Magnitude), 并根据相应量程范围乘以系数转换为以重力加速度g为单位
  float Acc_RMS = AccRange_Coff * sqrt(square_sum) / 32768.0;   // 单位 g

  if (Acc_RMS >= ACCELERATION_THRESHOLD){
    ret = 1;
    Serial.println();
    Serial.print("Acc_RMS = ");
    Serial.print(Acc_RMS);
    Serial.println("g");
  }
  // else{
  //   Serial.println();
  //   Serial.print("Below Threshold 2g, Acc_RMS = ");
  //   Serial.print(Acc_RMS);
  //   Serial.println("g");
  // }

  return ret;
}

void Acquisition() {
  for (int i = 0; i < 6; i++) {    //New改: 一组6个数据, 前3个加速度, 后3个角速度(陀螺仪)
    IMUData[i] = 0;
  }

  for (int c = 0; c < SMOTH_COUNT; c++) {       // New改: 每次采样连续读SMOTH_COUNT次MPU6050, 且将这些读数全部相加后作为输出, 相当于平滑且扩大了SMOTH_COUNT倍, 原代码为5    
    // read raw accel/gyro measurements from device
    accelgyro.getMotion6(&ax, &ay, &az, &gx, &gy, &gz);
    SampleData[0] = ax;
    SampleData[1] = ay;
    SampleData[2] = az;
    SampleData[3] = gx;
    SampleData[4] = gy;
    SampleData[5] = gz;

    for (int i = 0; i < 6; i++)  {    //New改: 一组6个数据, 前3个加速度, 后3个角速度(陀螺仪)
      IMUData[i] += SampleData[i];
    }
  }

  //sprintf(SerialBuffer, "%+5d\t%+5d\t%+5d | %+5d\t%+5d\t%+5d\r\n", IMUData[0], IMUData[1], IMUData[2], IMUData[3], IMUData[4], IMUData[5]);   //New改: 一组6个数据, 前3个加速度, 后3个角速度(陀螺仪)
  //Serial.print(SerialBuffer);
}

// void Send()
// {
//   //Channel.Send(RecordCount, (unsigned char*)IMUData, 24);   // New改: 每个IMU数据为4byte, 6个数据24 bytes
//    Channel.Send(DEST_IP, DEST_PORT, RecordCount, (unsigned char*)IMUData, 24);   // New改: 每个IMU数据为4byte, 6个数据24 bytes
// }

bool Inference()
{
  bool ret = false;

  // Copy Data to Model Input
  Serial.println("Copying Data to Model Inpute...");    //New add
  for (int k = 0; k < SAMPLE_COUNT; k++) {
    for (int i = 0; i < 6; i++)  {
      tflInputTensor->data.f[k * 6 + i] = Samples[i][k];
    }
  }

  // Start Inference
  Serial.println("Inferencing...");   //New add
  unsigned long t = millis();         // New change，replaced by mills()
  TfLiteStatus invokeStatus = tflInterpreter->Invoke();
  unsigned long t_int = millis() - t;

  // Check Result
  Serial.println("Checking Result...");   //New add
  if (invokeStatus == kTfLiteOk) {
    sprintf(SerialBuffer, "Invoke In %8.2f(ms)", t_int);
    Serial.println(SerialBuffer);

    ret = true;
  }
  else {
    Serial.println("Invoke failed!");
  }

  return ret;
}

void Result()
{  
  Serial.print("P_Threshold = ");   //New add
  Serial.println(P_Threshold);      //New add


  // 4 outputs
  auto p_fall = tflOutputTensor->data.f[0];
  auto p_jump = tflOutputTensor->data.f[1];
  auto p_jog = tflOutputTensor->data.f[2];
  auto p_sitdown = tflOutputTensor->data.f[3];
  // 6 outputs
  // auto p_fall = tflOutputTensor->data.f[0];
  // auto p_jump = tflOutputTensor->data.f[1];
  // auto p_jog = tflOutputTensor->data.f[2];
  // auto p_walk = tflOutputTensor->data.f[3];
  // auto p_sitdown = tflOutputTensor->data.f[4];
  // auto p_downstairs = tflOutputTensor->data.f[5];
  // 8 outputs
  // auto p_fall = tflOutputTensor->data.f[0];
  // auto p_jump = tflOutputTensor->data.f[1];
  // auto p_jog = tflOutputTensor->data.f[2];
  // auto p_walk = tflOutputTensor->data.f[3];
  // auto p_sitdown = tflOutputTensor->data.f[4];
  // auto p_situp = tflOutputTensor->data.f[5];
  // auto p_downstairs = tflOutputTensor->data.f[6];
  // auto p_upstairs = tflOutputTensor->data.f[7];

  ShowOutput();

  // if (p_fall > P_Threshold)
  // {
  //   Led.Red();
  //   Serial.println("Red Led On: Fall");   //New add
  // }
  if (p_fall > P_Threshold)
  {
    Led.Red();
    Serial.println("Red Led On: Fall");   //New add
  }
  else if (p_jump > P_Threshold)
  {
    Led.Yellow();
    Serial.println("Yellow Led On: Jump");   //New add
  }
  else if (p_jog > P_Threshold)
  {
    Led.Green();   
    Serial.println("Green Led On: Jog");    //New add
  }
  // else if (p_walk > P_Threshold)
  // {
  //   for (int i = 0; i < 2; i++)  {
  //     Led.Green();
  //     delay(1000);
  //     Led.Blank();
  //     delay(1000);
  //   }    
  //   Serial.println("Green Led Flash 2 times: Walk");    //New add    
  // }
  else if (p_sitdown > P_Threshold)
  {
    Led.Red();
    Led.Yellow();    
    Serial.println("Red & Yellow Led On: Sitdown");    //New add    
  }
  // else if (p_situp > P_Threshold)
  // {
  //   Led.Red();
  //   Led.Green();    
  //   Serial.println("Red & Green Led On: Situp");    //New add    
  // }
  // else if (p_downstairs > P_Threshold)
  // {
  //   Led.Yellow();
  //   Led.Green(); 
  //   Serial.println("Yellow & Green Led On: Downstairs");    //New add    
  // }
  // else if (p_upstairs > P_Threshold)
  // {
  //   Led.Yellow();
  //   Led.Red();
  //   Led.Green(); 
  //   Serial.println("Yellow & Red & Green Led On: Upstairs");    //New add    
  // }
  else
  {
    Led.Blank();
    Serial.println("No Led On! ");   //New add
  }
  Serial.println();
}

void ShowOutput()
{
  for (int i = 0; i < NUM_GESTURES; i++)
  {
    // sprintf(SerialBuffer, "%s: %8.6f", GESTURES[i], tflOutputTensor->data.f[i]);
    sprintf(SerialBuffer, "%s: %6.4f", GESTURES[i], tflOutputTensor->data.f[i]);
    Serial.println(SerialBuffer);
    // sprintf(ResultBuffer[i], "%s: %8.6f; ", GESTURES[i], tflOutputTensor->data.f[i]);   // New add for ResultDisplay.py 
    sprintf(ResultBuffer[i], "%s: %6.4f; ", GESTURES[i], tflOutputTensor->data.f[i]);   // New add for ResultDisplay.py
  }
  Channel.Send(DEST_IP, DEST_PORT2, (unsigned char*)ResultBuffer, NUM_GESTURES * 20);    // New add for ResultDisplay.py
}
