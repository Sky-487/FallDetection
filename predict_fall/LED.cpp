//原代码BLUE_PIN 改为GREEN_PIN，LED::green()、LED::Red()和LED::Blank()中改为高电平点亮。
#include "LED.h"
#include <Arduino.h>

LED::LED()
{
    pinMode(LED_PIN, OUTPUT);
    pinMode(GREEN_PIN, OUTPUT);
    pinMode(RED_PIN, OUTPUT);
    pinMode(YELLOW_PIN, OUTPUT);
              
    this->Off();
    this->Blank();
}

void LED::On()
{
    digitalWrite(LED_PIN, HIGH);
    this->State = true;
}

void LED::Off()
{
    digitalWrite(LED_PIN, LOW);
    this->State = true;
}
    
void LED::Toggle()
{
    this->State ? this->Off() : this->On();
}

void LED::Yellow()
{
    //digitalWrite(RED_PIN, LOW);
    digitalWrite(YELLOW_PIN, HIGH);
}

void LED::Green()
{
    digitalWrite(GREEN_PIN, HIGH);
}

void LED::Red()
{
    //digitalWrite(GREEN_PIN, LOW);
    digitalWrite(RED_PIN, HIGH);
}

void LED::Blank()
{
    digitalWrite(YELLOW_PIN, LOW);
    digitalWrite(GREEN_PIN, LOW);
    digitalWrite(RED_PIN, LOW);  
}

LED Led;
