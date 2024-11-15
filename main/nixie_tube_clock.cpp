#include <Arduino.h>

#include "nixie_tube_clock.h"

namespace{
  const bool Pressed = true, Unpressed = false;
  const int button_input_mode_back_pin_ = A1;//不能只寫'1'
  const int button_input_add_front_pin_ = A2;
  const int led_output_pin_;/*與74HC595 小時的'大平台'共用*/
}


NixieTubeClock::NixieTubeClock()
  : real_time_clock_(Ds1302(4/*enable_pin*/, 2/*clock_pin*/, 3/*data_pin*/)), 
    current_mode_(ClockMode::Normal),
    last_clean_tube_time_millsecond_(millis()),
    time_registers_{ ShiftRegister(6, 7, 5)/*秒*/, 
      ShiftRegister(9, 10, 8)/*分*/, ShiftRegister(12, 13, 11)/*時*/ }{
        
    RandomizeNextCleaningTime();
    SetRealTimeClock();
}

void NixieTubeClock::Setup(){
  pinMode(button_input_mode_back_pin_, INPUT);
  pinMode(button_input_add_front_pin_, INPUT);
}

void NixieTubeClock::CheckButtonStatus(){
  ProcessCleanTube();
  ProcessSettingTime();
}

void NixieTubeClock::RandomizeNextCleaningTime(){// 隨機選擇15至 30 秒 的時間
  target_time_seconds_ = random(15, 30);
  Serial.println("target_time_seconds_ : " + String(target_time_seconds_));
}

void NixieTubeClock::ProcessCleanTube(){
  bool back_button = IsButtonPressed(button_input_mode_back_pin_);
  bool front_button = IsButtonPressed(button_input_add_front_pin_);
  
  CheckIfExecuteCleanTubeMode();//手動清輝光管// cleaner_.ProcessCleanTube(back_button, front_button);
  

  // if(current_mode_ != ClockMode::Normal){
  //   return;//只在正常模式下清理管子
  // }
  AutoCleanTube();//固定時間清輝光管
}

void NixieTubeClock::UpdateTimeIfNextSecond(){
  static uint8_t last_second_ = 0;
  Ds1302::DateTime now;
  real_time_clock_.getDateTime(&now);// get the current time
  if (last_second_ == now.second){
    return;
  }
  static bool current_led_turn_on_ = false;
  if(current_led_turn_on_){
  Serial.println("ON");

    digitalWrite(led_output_pin_, HIGH);
  }
  else{
  Serial.println("OFF");
    digitalWrite(led_output_pin_, LOW);
  }
  current_led_turn_on_ = !current_led_turn_on_;

  last_second_ = now.second;
  if(current_mode_ != ClockMode::Normal){
    return;
  }
  
  SetTime(now);
  Serial.println(String(now.hour) + ":" + String(now.minute) + ":" + String(now.second));

}

void NixieTubeClock::SetRealTimeClock(){
  real_time_clock_.init();
  if (real_time_clock_.isHalted()){// if is halted, set a date-time
    Serial.println("RTC is halted. Setting time...");
    Ds1302::DateTime dt = {.year = 24, .month = Ds1302::MONTH_OCT,
      .day = 3,.hour = 2, .minute = 2, .second = 22, .dow = Ds1302::DOW_TUE};
    real_time_clock_.setDateTime(&dt);
  }
}

void NixieTubeClock::CheckIfExecuteCleanTubeMode(){
  bool back_button = IsButtonPressed(button_input_mode_back_pin_);
  bool front_button = IsButtonPressed(button_input_add_front_pin_);

  static unsigned long press_2_button_millsecond_ = 0; // 記錄按下的開始時間
  static bool is_cleaning_mode_ = false; // 是否處於輝光管清除模式
  unsigned long press_duration;
  // 檢測是否同時按下按鈕
  if (back_button == Pressed && front_button == Pressed) {
    if (press_2_button_millsecond_ == 0) { // 如果是剛開始按下，記錄時間
      press_2_button_millsecond_ = millis();
      Serial.println("press_2_button_millsecond_: " + String(press_2_button_millsecond_));
    } 
    else {
      press_duration = millis() - press_2_button_millsecond_;
      Serial.println("press_duration: " + String(press_duration));

      if (press_duration > 1000 && !is_cleaning_mode_) { // 如果按下時間超過1秒，且不在清除模式中
        is_cleaning_mode_ = true; // 進入清除模式
        last_clean_tube_time_millsecond_ = 0;
        CleanTube();
        is_cleaning_mode_ = false; // 清除模式結束
        press_2_button_millsecond_ = 0; // 重置按下時間，以便下次按下
      }
    }
  } else {
    press_2_button_millsecond_ = 0; // 如果按鈕沒有同時按下，重置按下時間
  }

}

void NixieTubeClock::CleanTube(){
  // 啟動輝光管保護，點亮一段時間
  int current_clean_time = 0;        
  float clean_delay_time = 100;
  const int tube_clean_time = 25;
  //由管子的後到前顯示數字  
  const byte tube_show_numbers[10] = {11, 66, 22, 55, 0, 44, 77, 99, 88, 33};
  while(current_clean_time < tube_clean_time){
    int show_num = tube_show_numbers[current_clean_time % 10];
    Ds1302::DateTime now;
    now.second = show_num;
    now.minute = show_num;
    now.hour = show_num;
    SetTime(now);

    delay(clean_delay_time);
    clean_delay_time -= 0.9;
    current_clean_time++;
  }
  UpdateTimeIfNextSecond();//清理完管子後, 馬上顯示當前時間
}



void NixieTubeClock::AutoCleanTube(){
  if(current_mode_ != ClockMode::Normal){
    return;//只在正常模式下清理管子
  }
  const unsigned long current_time = millis();
  const unsigned long time_diff_seconds = (current_time - last_clean_tube_time_millsecond_) / 1000/*轉成秒*/;
  // 檢查時間差是否達到清理時間
  if (time_diff_seconds == target_time_seconds_){
    CleanTube();
    last_clean_tube_time_millsecond_ = millis();
    RandomizeNextCleaningTime();
  }
}


void NixieTubeClock::ProcessSettingTime(){
  bool back_set_btn = IsButtonPressed(button_input_mode_back_pin_);
  bool front_add_btn = IsButtonPressed(button_input_add_front_pin_);

  if(current_mode_ == ClockMode::Normal){
    CheckIfExecuteSettingTimeMode(back_set_btn, front_add_btn);
  }
  //如果上述檢查沒有進入設定時間模式, 這裡不再檢查是否切換時分秒
  if(current_mode_ == ClockMode::Normal){
    return;
  }
  CheckIfSwitchSettingTime(back_set_btn);//是否second minute hour 切換
  BlinkTube();//閃爍當前設定的管子
  ListenAddButtonToSetTime(back_set_btn, front_add_btn);//聆聽front add調整時間
}


void NixieTubeClock::CheckIfExecuteSettingTimeMode(bool back_setting_btn, bool front_add_btn){
  // 如果在正常模式下長按back_button 且add按鈕沒按(清理模式會按著2顆按鈕)，則進入設定模式
  if (back_setting_btn == Pressed && front_add_btn == Unpressed){

    const unsigned long press_duration = millis() - setting_mode_press_time_;
    if (setting_mode_press_time_ == 0) {
      setting_mode_press_time_ = millis(); // 開始計時
      Serial.println("press setting button duration :" + String(press_duration));
      
    } else if (press_duration > 3000) { // 長按超過3秒
      Serial.println("Entering setting second mode");
      last_blink_millsecond_ = millis();
      current_mode_ = ClockMode::SettingSecond; // 進入設定秒數模式
      last_back_set_btn_ = back_setting_btn;
      setting_mode_unpress_millsecond_ = millis();
      setting_mode_press_time_ = 0; // 重置計時
    }
  } else if (back_setting_btn == Unpressed && setting_mode_press_time_ != 0){
    setting_mode_press_time_ = 0; // 重置計時，因為按鈕已經釋放
  }
}

// 在設定模式下切換設定項目, second->minute->hour->second...
void NixieTubeClock::CheckIfSwitchSettingTime(const bool back_set_btn){
  if(back_set_btn == last_back_set_btn_ || back_set_btn == Unpressed){
    last_back_set_btn_ = back_set_btn;
    return;
  }
  setting_mode_unpress_millsecond_ = millis();

  // 使用單一語句循環切換模式
  current_mode_ = static_cast<ClockMode>((current_mode_ + 1 - SettingSecond) % 3 + SettingSecond);
  
  // 循環通過設定模式
  if (current_mode_ == SettingSecond){
    Serial.println("Switch to set minute mode");
    delay(5);//防抖動
  }
  else if (current_mode_ == SettingMinute){
    Serial.println("Switch to set hour mode");
    delay(5);//防抖動
  } 
  else if (current_mode_ == SettingHour){
    Serial.println("Switch to set second mode");
  } 

  delay(5);//防抖動
  last_blink_millsecond_ = millis();
  last_back_set_btn_ = back_set_btn;
}

// 經過８秒沒按任何按鍵, 就變成正常模式
void NixieTubeClock::ListenAddButtonToSetTime(const bool back_button, const bool front_add_btn){
  if(front_add_btn == last_front_add_btn_ || front_add_btn == Unpressed){
    unsigned long current_time = millis();
    unsigned long time_diff_s = current_time - setting_mode_unpress_millsecond_;
    if(time_diff_s > 8000/*8秒*/){
      setting_mode_unpress_millsecond_ = millis();
      current_mode_ = ClockMode::Normal;
      Serial.println("Set back to normal");
    }
    last_front_add_btn_ = front_add_btn;
    return;
  }
  last_front_add_btn_ = front_add_btn;

  if (front_add_btn == Pressed){
    setting_mode_unpress_millsecond_ = millis();//有按按鍵,更新上次按下的時間
    AdjustTime();
    delay(1);//防抖動
  }
}

//閃爍當前設定的時間, 但當切換的瞬間, 需要將上一個快速顯示回去, 不能不顯示
void NixieTubeClock::BlinkTube(){ 
  unsigned long current_time = millis();
  unsigned long current_off_time_millisecond_ = 
    (current_time - last_blink_millsecond_);
  Ds1302::DateTime now;
  real_time_clock_.getDateTime(&now);
  if(current_off_time_millisecond_ % 1000 > 500){
    if (current_mode_ == SettingSecond){
      SetTimeUnit(TimeUnit::Second, now.second);
    } else if (current_mode_ == SettingMinute){
      SetTimeUnit(TimeUnit::Minut, now.minute);
    } else if (current_mode_ == SettingHour){
      SetTimeUnit(TimeUnit::Hour, now.hour);
    }
  }
  else{
    if (current_mode_ == SettingSecond){
      now.second = 255;
      SetTime(now);
    } else if (current_mode_ == SettingMinute){
      now.minute = 255;
      SetTime(now);
    } else if (current_mode_ == SettingHour){
      now.hour = 255;
      SetTime(now);
    }
  }
}

void NixieTubeClock::AdjustTime(){
  Ds1302::DateTime now;
  real_time_clock_.getDateTime(&now);
  if (current_mode_ == SettingSecond) {
    now.second = 0; // 將秒歸零
    SetTimeUnit(TimeUnit::Second, now.second);
    Serial.println("Set minut to: 0");
  } else if (current_mode_ == SettingMinute) {
    now.minute = (now.minute + 1) % 60; // 分鐘加一
    SetTimeUnit(TimeUnit::Minut, now.minute);
    Serial.println("Set minut to: " + String(now.minute));
  } else if (current_mode_ == SettingHour) {
    now.hour = (now.hour + 1) % 24; // 時數加一
    SetTimeUnit(TimeUnit::Hour, now.hour);
    Serial.println("Set hour to: " + String(now.hour));
  }

  real_time_clock_.setDateTime(&now); // 更新時間
}

void NixieTubeClock::SetTimeUnit(const TimeUnit unit, const int now){
  time_registers_[unit].SetClockTime(now);
}

void NixieTubeClock::SetTime(const Ds1302::DateTime& now){
  time_registers_[TimeUnit::Second].SetClockTime(now.second);
  time_registers_[TimeUnit::Minut].SetClockTime(now.minute);
  time_registers_[TimeUnit::Hour].SetClockTime(now.hour);
}

bool NixieTubeClock::IsButtonPressed(int pin) {
    return analogRead(pin) < 50;
}

bool NixieTubeClock::IsButtonReleased(int pin) {
    return analogRead(pin) >= 50;
}