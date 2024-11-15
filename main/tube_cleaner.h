#ifndef TUBE_CLEANER_H
#define TUBE_CLEANER_H

#include <Arduino.h>


class TubeCleaner {

private:
    void CleanTube();  // 清潔操作
    void RandomizeNextCleaningTime();  // 設定下一次清潔的隨機時間
    unsigned long last_clean_tube_time_millsecond_ = millis();

public:
    void ProcessCleanTube(bool back_button, bool front_button){
      static unsigned long press_2_button_millsecond_ = 0; // 記錄按下的開始時間
      static bool is_cleaning_mode_ = false; // 是否處於輝光管清除模式
      unsigned long press_duration;
      // 檢測是否同時按下按鈕
      if (back_button == true && front_button == true) {
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


    void AutoCleanTube(){
      // const unsigned long current_time = millis();
      // const unsigned long time_diff_seconds = (current_time - last_clean_tube_time_millsecond_) / 1000/*轉成秒*/;
      // // 檢查時間差是否達到清理時間
      // if (time_diff_seconds == target_time_seconds_){
      //   CleanTube();
      //   last_clean_tube_time_millsecond_ = millis();
      //   RandomizeNextCleaningTime();
      // }
    }
    void ManualCleanTube(bool back_btn, bool front_btn);  // 手動清潔功能
};

#endif // TUBE_CLEANER_H
