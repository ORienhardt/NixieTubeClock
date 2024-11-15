#ifndef NIXIE_TUBE_CLOCK_INO
#define NIXIE_TUBE_CLOCK_INO
#include <Arduino.h>
#include <Ds1302.h>

#include "shift_register.h"
#include "tube_cleaner.h"


enum ClockMode { Normal, SettingSecond, SettingMinute, SettingHour };
enum TimeUnit {Second, Minut, Hour};

class NixieTubeClock{
  Ds1302 real_time_clock_;
  ShiftRegister time_registers_[3];
  friend class TubeCleaner;

  TubeCleaner cleaner_;
  unsigned long last_clean_tube_time_millsecond_;
  unsigned long target_time_seconds_ = 0;  // 目標時間，單位為秒

  //*************************************設定時間*************************
  bool last_back_set_btn_;
  bool last_front_add_btn_;
  unsigned long setting_mode_press_time_ = millis(); // 記錄進入設定模式按鈕按下的時間
  //*********************************設定時間******************************
  unsigned long last_blink_millsecond_ = 0;
  unsigned long setting_mode_unpress_millsecond_ = 0;

  ClockMode current_mode_;
  
  void RandomizeNextCleaningTime();
  void AutoCleanTube();
  void CleanTube();
  void CheckIfExecuteCleanTubeMode();
  void SetRealTimeClock();
  void ProcessSettingTime();
  void ProcessCleanTube();
  void CheckIfExecuteSettingTimeMode(bool back_setting_btn, bool front_add_btn);
  void CheckIfSwitchSettingTime(const bool back_set_btn);
  void ListenAddButtonToSetTime(const bool back_button, const bool front_add_btn);
  void BlinkTube();
  void AdjustTime();
  void SetTimeUnit(const TimeUnit unit, const int now);
  void SetTime(const Ds1302::DateTime& now);
  bool IsButtonPressed(int pin);
  bool IsButtonReleased(int pin);

 public:
  NixieTubeClock();
  void Setup();
  void CheckButtonStatus();
  void UpdateTimeIfNextSecond();
};

#endif // NIXIE_TUBE_CLOCK_INO