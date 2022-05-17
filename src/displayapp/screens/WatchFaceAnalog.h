#pragma once

#include <lvgl/src/lv_core/lv_obj.h>
#include <chrono>
#include <cstdint>
#include <memory>
#include "displayapp/screens/Screen.h"
#include "displayapp/screens/ScreenList.h"
#include "components/datetime/DateTimeController.h"
#include "components/battery/BatteryController.h"
#include "components/ble/BleController.h"
#include "components/ble/NotificationManager.h"

namespace Pinetime {
  namespace Controllers {
    class Settings;
    class Battery;
    class Ble;
    class NotificationManager;
    class MotionController;
  }
  namespace Applications {
    namespace Screens {

      class WatchFaceAnalog : public Screen {
      public:
        WatchFaceAnalog(DisplayApp* app,
                        Controllers::DateTime& dateTimeController,
                        Controllers::Battery& batteryController,
                        Controllers::Ble& bleController,
                        Controllers::NotificationManager& notificationManager,
                        Controllers::Settings& settingsController,
                        Controllers::MotionController& motionController);

        ~WatchFaceAnalog() override;

        void Refresh() override;

      private:
        uint8_t sHour, sMinute, sSecond;

        Pinetime::Controllers::DateTime::Months currentMonth = Pinetime::Controllers::DateTime::Months::Unknown;
        Pinetime::Controllers::DateTime::Days currentDayOfWeek = Pinetime::Controllers::DateTime::Days::Unknown;
        uint8_t currentDay = 0;

        DirtyValue<uint8_t> batteryPercentRemaining {0};
        DirtyValue<bool> isCharging {};
        DirtyValue<std::chrono::time_point<std::chrono::system_clock, std::chrono::nanoseconds>> currentDateTime;
        DirtyValue<bool> notificationState {false};
        DirtyValue<uint32_t> stepCount {};
        DirtyValue<bool> motionSensorOk {};



        lv_obj_t* hour_body;
        lv_obj_t* hour_body_trace;
        lv_obj_t* minute_body;
        lv_obj_t* minute_body_trace;
        lv_obj_t* second_body;

        lv_point_t hour_point[2];
        lv_point_t hour_point_trace[2];
        lv_point_t minute_point[2];
        lv_point_t minute_point_trace[2];
        lv_point_t second_point[2];

        lv_style_t hour_line_style;
        lv_style_t hour_line_style_trace;
        lv_style_t minute_line_style;
        lv_style_t minute_line_style_trace;
        lv_style_t second_line_style;

        lv_obj_t* bleIcon;
        lv_obj_t* label_time_hour;
        lv_obj_t* label_time_minute;
        lv_obj_t* label_time_second;
        lv_obj_t* label_date_day;
        lv_obj_t* batteryIcon;
        lv_obj_t* notificationIcon;
        lv_obj_t* stepIcon;
        lv_obj_t* stepValue;

        const Controllers::DateTime& dateTimeController;
        Controllers::Battery& batteryController;
        Controllers::Ble& bleController;
        Controllers::NotificationManager& notificationManager;
        Controllers::Settings& settingsController;
        Controllers::MotionController& motionController;

        void UpdateClock();
        void SetBatteryIcon();
        int HourFormat12(int);

        lv_task_t* taskRefresh;
      };
    }
  }
}
