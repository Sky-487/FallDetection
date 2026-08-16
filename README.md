# FallDetection
These codes is for a wearable waist fall detection device.  
The reference code comes from a set of gesture recognition codes: https://github.com/mushroomcloud-cc/tinyml-workshop

1 Hardware 
ESP32 is chosen as microprocessor for its low-cost, low-power, high-performance and Wi-Fi support.
MPU6050 is chosen as motion sensor for  its hig-precision, low-power, and easy-to-integrate.

2 Software development environment
Arduino (C/C++) for ESP32
VS Code (Python) for Host

3 Core technique
TinyML

4 code directory
Host directory--data文件夹--data.csv (all data)
		  |			|--fall.csv (fall data)	
		  |			|--jog.csv (jog data）
		  | 			|--jump.csv（jump data）
		  |			 --sitdown.csv（sitdown data）
		  |--	model.tflite（ TinyML model）	
		  |--	model.h（TinyML model for ESP32）
		  |--	SensorProcess.py （UDP server, to receive the data from UDP client）	
		   --	tinyml.py （model training and converting）			
			
predict_fall 文件夹--I2Cdev.cpp，I2Cdev.h （I2Cdev driving）
			      |--LED.cpp，LED.h （LED driving）
			      |--MPU6050.cpp，MPU6050.h （MPU6050 driving）
			      |--UDPChannel.cpp，UDPChannel.h （UDP client）
			      |--predict_fall.ino（fall detection）
			       --model.h（TinyML model for ESP32）
