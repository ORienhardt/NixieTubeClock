#include <Arduino.h>
#include <Ds1302.h>
#include "shift_register.h"

enum ClockMode { Nornal, SettingSecond, SettingMinute, SettingHour };
static bool Pressed = true, Unpressed = false;
const int led_output_pin_ = 12;//共用小時的大平台pin
bool current_led_turn_on_ = false;
ShiftRegister second_(6,5,7);
ShiftRegister minut_(9,10,8);
ShiftRegister hour_(12,13,11);

Ds1302 real_time_clock_(4/*enable_pin*/, 2/*clock_pin*/, 3/*data_pin*/);

const int button_input_mode_back_pin_ = A1;//不能只寫'1'
const int button_input_add_front_pin_ = A2;
ClockMode current_mode_;


//正常模式：更新時間
void SetRealTimeClock();
void UpdataTimeIfNextSecond();
void ProcessSettingClockMode();

//清理輝光管
void ProcessCleanTube(const bool back_btn, const bool front_btn);
void CheckIfExecuteCleanTubeMode(const bool back_btn, const bool front_btn);
void AutoCleanTube();
void CleanTube();

//檢查是否設定時間、更新時間
void ProcessSettingTime(const bool back_set_btn, const bool front_add_bton);
void CheckIfExecuteSettingTimeMode(const bool back_button, const bool front_button);
void CheckIfSwitchSettingTime(const bool back_button);
void ListenAddButtonToSetTime(const bool back_button, const bool front_button);
void AdjustTime();
void BlinkTube();
unsigned long last_clean_tube_time_millsecond_;


void setup(){
  Serial.begin(9600);
  last_clean_tube_time_millsecond_ = millis();
  current_mode_ = ClockMode::Nornal; // 當前設定模式
  RandomizeNextCleaning();
  SetRealTimeClock();
  pinMode(button_input_mode_back_pin_, INPUT);
  pinMode(button_input_add_front_pin_, INPUT);
}

void loop(){
  UpdataTimeIfNextSecond();
  const bool back_setting_btn = analogRead(button_input_mode_back_pin_) == 0;
  const bool front_adding_btn = analogRead(button_input_add_front_pin_) == 0;
  ProcessCleanTube(back_setting_btn, front_adding_btn);
  ProcessSettingTime(back_setting_btn, front_adding_btn);
  delay(20);
}

void ProcessCleanTube(const bool back_btn, const bool front_btn){
  CheckIfExecuteCleanTubeMode(back_btn, front_btn);//手動清輝光管
  AutoCleanTube();//固定時間清輝光管
}

void UpdataTimeIfNextSecond(){
  static uint8_t last_second_ = 0;
  Ds1302::DateTime now;
  real_time_clock_.getDateTime(&now);// get the current time
  if (last_second_ == now.second){
    return;
  }
  if(current_led_turn_on_){
    digitalWrite(led_output_pin_, HIGH);
  }
  else{
    digitalWrite(led_output_pin_, LOW);
  }
  current_led_turn_on_ = !current_led_turn_on_;

  last_second_ = now.second;
  if(current_mode_ != ClockMode::Nornal){
    return;
  }
  second_.SetClockTime(now.second);
  minut_.SetClockTime(now.minute);
  hour_.SetClockTime(now.hour);
}

void SetRealTimeClock(){
  real_time_clock_.init();
  if (real_time_clock_.isHalted()){// if is halted, set a date-time
    Serial.println("RTC is halted. Setting time...");
    Ds1302::DateTime dt = {.year = 24, .month = Ds1302::MONTH_OCT,
      .day = 3,.hour = 2, .minute = 2, .second = 22, .dow = Ds1302::DOW_TUE};
    real_time_clock_.setDateTime(&dt);
  }
}

unsigned long press_2_button_millsecond_ = 0; // 記錄按下的開始時間
unsigned long last_auto_clean_millsecond_ = 0; // 記錄上次清除時間 自動執行： 15 ~ 30分鐘
bool is_cleaning_mode_ = false; // 是否處於輝光管清除模式
static const int tube_clean_time_ = 100;    
const byte tube_show_numbers_[10] = {11, 66, 22, 55, 0, 44, 77, 99, 88, 33};


void CheckIfExecuteCleanTubeMode(const bool back_button, const bool front_button){
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
        last_auto_clean_millsecond_ = 0;// 如果手動清除, 則自動清除的計時也歸０
      }
    }
  } else {
    press_2_button_millsecond_ = 0; // 如果按鈕沒有同時按下，重置按下時間
  }

}

void CleanTube(){
  // 啟動輝光管保護，點亮一段時間
  int current_clean_time = 0;        
  float clean_delay_time = 100;
  bool on = false;
  while(current_clean_time < tube_clean_time_){
    int show_num = tube_show_numbers_[current_clean_time % 10];
    digitalWrite(led_output_pin_, on);
    on = !on;
    second_.SetClockTime(show_num);
    minut_.SetClockTime(show_num);
    hour_.SetClockTime(show_num);
    delay(clean_delay_time);
    clean_delay_time -= 0.9;
    current_clean_time++;
  }
}

unsigned long target_time_minutes_ = 0;  // 目標時間，單位為分鐘
void RandomizeNextCleaning(){// 隨機選擇15至30分鐘間3的倍數的時間
  int min_minutes = 15;
  int max_minutes = 30;
  int interval = 3;
  target_time_minutes_ = 
    random(min_minutes / interval, (max_minutes / interval) + 1) * interval;
}

void AutoCleanTube(){
  unsigned long current_time = millis();
  unsigned long time_diff_s = current_time - last_clean_tube_time_millsecond_;
  unsigned long time_diff_minutes = time_diff_s / 60000;

  // 檢查時間差是否達到清理時間
  if (time_diff_minutes == target_time_minutes_){
    CleanTube();
    last_clean_tube_time_millsecond_ = millis();
    RandomizeNextCleaning();
  }
}

unsigned long press_front_setting_button_millsecond_ = 0;
unsigned long press_back_add_button_millsecond_ = 0; 
//*************************************設定時間*************************
bool last_back_set_btn_;
bool last_front_add_btn_;
unsigned long setting_mode_press_time_ = 0; // 記錄進入設定模式按鈕按下的時間
//*********************************設定時間******************************
unsigned long last_blink_millsecond_ = 0;

void ProcessSettingTime(const bool back_set_btn, const bool front_add_btn){

  if(current_mode_ == ClockMode::Nornal){
    CheckIfExecuteSettingTimeMode(back_set_btn, front_add_btn);
  }
  //如果上述檢查沒有進入設定時間模式, 這裡不再檢查是否切換時分秒
  if(current_mode_ == ClockMode::Nornal){
    return;
  }
  CheckIfSwitchSettingTime(back_set_btn);//是否second minute hour 切換
  BlinkTube();//閃爍當前設定的管子
  ListenAddButtonToSetTime(back_set_btn, front_add_btn);//聆聽front add調整時間
}

unsigned long setting_mode_unpress_millsecond_ = 0;

void CheckIfExecuteSettingTimeMode(bool back_setting_btn, bool front_add_btn){
  // 如果在正常模式下長按back_button 且add按鈕沒按(清理模式會按著2顆按鈕)，則進入設定模式
  if (back_setting_btn == Pressed && front_add_btn == Unpressed){
    if (setting_mode_press_time_ == 0) {
      setting_mode_press_time_ = millis(); // 開始計時
    } else if (millis() - setting_mode_press_time_ > 3000) { // 長按超過3秒
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
void CheckIfSwitchSettingTime(const bool back_set_btn){
  if(back_set_btn == last_back_set_btn_ || back_set_btn == Unpressed){
    last_back_set_btn_ = back_set_btn;
    return;
  }
  setting_mode_unpress_millsecond_ = millis();
  delay(1); // 簡單的防抖動
  // 循環通過設定模式
  if (current_mode_ == SettingSecond){
    current_mode_ = SettingMinute;
    Serial.println("Switch to set minute mode");
  }
  else if (current_mode_ == SettingMinute){
    current_mode_ = SettingHour;
    Serial.println("Switch to set hour mode");
  } 
  else if (current_mode_ == SettingHour){
    current_mode_ = SettingSecond;
    Serial.println("Switch to set second mode");
  } 
  Serial.println("last_blink_millsecond_~~~"+String( last_blink_millsecond_));

  last_blink_millsecond_ = millis();
  last_back_set_btn_ = back_set_btn;
}

// 經過８秒沒按任何按鍵, 就變成正常模式
void ListenAddButtonToSetTime(const bool back_button, const bool front_add_btn){
  if(front_add_btn == last_front_add_btn_ || front_add_btn == Unpressed){
    unsigned long current_time = millis();
    unsigned long time_diff_s = current_time - setting_mode_unpress_millsecond_;
    if(time_diff_s > 8000/*8秒*/){
      setting_mode_unpress_millsecond_ = millis();
      current_mode_ = ClockMode::Nornal;
      Serial.println("Set back to normal");
    }
    last_front_add_btn_ = front_add_btn;
    return;
  }
  last_front_add_btn_ = front_add_btn;

  if (front_add_btn == Pressed){
    setting_mode_unpress_millsecond_ = millis();//有按按鍵,更新上次按下的時間
    AdjustTime();
    delay(1); // 簡單的防抖動
  }
}

//閃爍當前設定的時間, 但當切換的瞬間, 需要將上一個快速顯示回去, 不能不顯示
void BlinkTube(){ 
  unsigned long current_time = millis();
  unsigned long current_off_time_millisecond_ = 
    (current_time - last_blink_millsecond_);
  Ds1302::DateTime now;
  real_time_clock_.getDateTime(&now);
  if(current_off_time_millisecond_ % 1000 > 500){
    if (current_mode_ == SettingSecond){
      second_.SetClockTime(now.second);
    } else if (current_mode_ == SettingMinute){
      minut_.SetClockTime(now.minute);
    } else if (current_mode_ == SettingHour){
      hour_.SetClockTime(now.hour);
    }
  }
  else{
    if (current_mode_ == SettingSecond){
      second_.SetClockTime(-1);
      minut_.SetClockTime(now.minute);
      hour_.SetClockTime(now.hour);
    } else if (current_mode_ == SettingMinute){
      second_.SetClockTime(now.second);
      hour_.SetClockTime(now.hour);
      minut_.SetClockTime(-1);
    } else if (current_mode_ == SettingHour){
      second_.SetClockTime(now.second);
      minut_.SetClockTime(now.minute);
      hour_.SetClockTime(-1);
    }
  }
}

void AdjustTime(){
  Ds1302::DateTime now;
  real_time_clock_.getDateTime(&now);
  if (current_mode_ == SettingSecond) {
    now.second = 0; // 將秒歸零
    second_.SetClockTime(now.second);
    Serial.println("Set minut to: 0");
  } else if (current_mode_ == SettingMinute) {
    now.minute = (now.minute + 1) % 60; // 分鐘加一
    minut_.SetClockTime(now.minute);
    Serial.println("Set minut to: " + String(now.minute));
  } else if (current_mode_ == SettingHour) {
    now.hour = (now.hour + 1) % 24; // 時數加一
    hour_.SetClockTime(now.hour);
    Serial.println("Set hour to: " + String(now.hour));
  }

  real_time_clock_.setDateTime(&now); // 更新時間
}
