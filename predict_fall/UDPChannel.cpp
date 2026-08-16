#include "UDPChannel.h"

#include "LED.h"
#include <WiFi.h>


UDPChannel::UDPChannel()
{
  
}

void UDPChannel::ConnectAP(const char* ssid, const char* pwd)
{
    // WiFi.softAP(AP_SSID, AP_PWD);
    WiFi.begin(ssid, pwd);

    Serial.write("Waiting for connect to AP");
    while (!WiFi.isConnected())
    {
        Led.Toggle();
        Serial.write('.');
    }

    Led.Off();
    Serial.println("");
}

//void UDPChannel::Send(int count, unsigned char* buf, int len)  
// New改: 255.255.255.255为受限广播地址, 某些网络中会受抑制而丢包严重，改为PC端具体地址或子网广播地址。Send函数增加目标地址和端口参数
void UDPChannel::Send(const char* destipaddr, unsigned int destport, int count, unsigned char* buf, int len)  
{
    //UDP.beginPacket("255.255.255.255", 8000);
    UDP.beginPacket(destipaddr, destport);   // New change: 255.255.255.255为受限广播地址, 某些网络中会受抑制而丢包严重，改为PC端具体地址或子网广播地址
    UDP.write((unsigned char*)&count, 4);
    UDP.write(buf, len);
    UDP.endPacket();
}

// New add for ResultDisplay.py
void UDPChannel::Send(const char* destipaddr, unsigned int destport, unsigned char* buf, int len)  
{
    //UDP.beginPacket("255.255.255.255", 8000);
    UDP.beginPacket(destipaddr, destport);   // New change: 255.255.255.255为受限广播地址, 某些网络中会受抑制而丢包严重，改为PC端具体地址或子网广播地址
    UDP.write(buf, len);
    UDP.endPacket();
}
