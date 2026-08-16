#ifndef _UDP_CHANNEL_H
#define _UDP_CHANNEL_H

#include <WiFiUdp.h>

class UDPChannel
{
public:
  UDPChannel();

  void ConnectAP(const char* ssid, const char* pwd);
  //void Send(int count, unsigned char* buf, int len);
  void Send(const char* destipaddr, unsigned int destport, int count, unsigned char* buf, int len);   // New change
  void Send(const char* destipaddr, unsigned int destport, unsigned char* buf, int len);   // New add for ResultDisplay.py
  
private:
  WiFiUDP UDP;
};

#endif
