#include <Arduino.h>

#include "shift_register.h"
#include "nixie_tube_clock.h"


NixieTubeClock nixie_tube_clock_;

void setup(){
  Serial.begin(9600);
  nixie_tube_clock_.Setup();
}

void loop(){
  nixie_tube_clock_.UpdateTimeIfNextSecond();
  nixie_tube_clock_.CheckButtonStatus();
  delay(10);
}