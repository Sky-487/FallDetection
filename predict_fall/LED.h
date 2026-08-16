//管脚根据FireBeetle Board-ESP32实际连接修改。Blue()改Green()
#ifndef _LED_H_
#define _LED_H_

const int LED_PIN = 2;      // GPIO2, =D9(板载LED, FireBeetle), =D2(板载LED, ESP32_30Pin)
// const int GREEN_PIN = 25;   // GPIO25, =D25(/A8, ESP32_30Pin)
// const int RED_PIN = 26;     // GPIO26, =D26(/A9, ESP32_30Pin)
// const int GREEN_PIN = 13;   // GPIO13, =D7(FireBeetle)
// const int RED_PIN = 5;      // GPIO5, =D8(FireBeetle)
const int YELLOW_PIN = 5;      // GPIO5, =D8(FireBeetle)
const int GREEN_PIN = 26;      // GPIO9, =D3(FireBeetle)
const int RED_PIN = 27;       // GPIO10, =D4(FireBeetle)

class LED {
public:
    LED();
    
    void On();
    void Off();
    void Toggle();

    void Yellow();
    void Green();
    void Red();
    void Blank();
private:
    bool State;
};

extern LED Led;

#endif // _LED_H_
