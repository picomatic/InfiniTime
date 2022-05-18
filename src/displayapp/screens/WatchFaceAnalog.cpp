#include "displayapp/screens/WatchFaceAnalog.h"
#include <lvgl/lvgl.h>
#include "displayapp/screens/BatteryIcon.h"
#include "displayapp/screens/BleIcon.h"
#include "displayapp/screens/Symbols.h"
#include "displayapp/screens/NotificationIcon.h"
#include "components/settings/Settings.h"
#include "components/motion/MotionController.h"

LV_IMG_DECLARE(bg_clock);

using namespace Pinetime::Applications::Screens;

namespace {
constexpr int16_t HourLength = 70;
constexpr int16_t MinuteLength = 90;
constexpr int16_t SecondLength = 110;

// sin(90) = 1 so the value of _lv_trigo_sin(90) is the scaling factor
const auto LV_TRIG_SCALE = _lv_trigo_sin(90);

int16_t Cosine(int16_t angle) {
  return _lv_trigo_sin(angle + 90);
}

int16_t Sine(int16_t angle) {
  return _lv_trigo_sin(angle);
}

int16_t CoordinateXRelocate(int16_t x) {
  return (x + LV_HOR_RES / 2);
}

int16_t CoordinateYRelocate(int16_t y) {
  return std::abs(y - LV_HOR_RES / 2);
}

lv_point_t CoordinateRelocate(int16_t radius, int16_t angle) {
  return lv_point_t{
    .x = CoordinateXRelocate(radius * static_cast<int32_t>(Sine(angle)) / LV_TRIG_SCALE),
    .y = CoordinateYRelocate(radius * static_cast<int32_t>(Cosine(angle)) / LV_TRIG_SCALE)
  };
}

}

WatchFaceAnalog::WatchFaceAnalog(Pinetime::Applications::DisplayApp* app,
                                 Controllers::DateTime& dateTimeController,
                                 Controllers::Battery& batteryController,
                                 Controllers::Ble& bleController,
                                 Controllers::NotificationManager& notificationManager,
                                 Controllers::Settings& settingsController,
                                 Controllers::MotionController& motionController)
  : Screen(app),
    currentDateTime {{}},
    dateTimeController {dateTimeController},
    batteryController {batteryController},
    bleController {bleController},
    notificationManager {notificationManager},
    settingsController {settingsController},
    motionController {motionController} {
    
  settingsController.SetClockFace(1);

  sHour = 99;
  sMinute = 99;
  sSecond = 99;

  lv_obj_t* bg_clock_img = lv_img_create(lv_scr_act(), NULL);
  lv_img_set_src(bg_clock_img, &bg_clock);
  lv_obj_align(bg_clock_img, NULL, LV_ALIGN_CENTER, 0, 0);
  
  label_date_day = lv_label_create(lv_scr_act(), NULL);
  lv_obj_set_style_local_text_color(label_date_day, LV_LABEL_PART_MAIN, LV_STATE_DEFAULT, lv_color_hex(0xFFFFFF));
  lv_label_set_text_fmt(label_date_day, "%s %02i", dateTimeController.DayOfWeekShortToString(), dateTimeController.Day());
  lv_label_set_align(label_date_day, LV_LABEL_ALIGN_CENTER);
  lv_obj_align(label_date_day, NULL, LV_ALIGN_CENTER, 50, 0);

  batteryIcon = lv_label_create(lv_scr_act(), nullptr);
  lv_label_set_text(batteryIcon, Symbols::batteryHalf);
  lv_obj_align(batteryIcon, NULL, LV_ALIGN_IN_BOTTOM_LEFT, 0, 0);
  lv_obj_set_auto_realign(batteryIcon, true);

  notificationIcon = lv_label_create(lv_scr_act(), NULL);
  lv_obj_set_style_local_text_color(notificationIcon, LV_LABEL_PART_MAIN, LV_STATE_DEFAULT, lv_color_hex(0xFF0000));
  lv_label_set_text(notificationIcon, NotificationIcon::GetIcon(false));
  lv_obj_align(notificationIcon, NULL, LV_ALIGN_IN_BOTTOM_LEFT, 35, 0);

  label_time_hour = lv_label_create(lv_scr_act(), nullptr);
  lv_obj_align(label_time_hour, lv_scr_act(), LV_ALIGN_IN_TOP_LEFT, 0, 0);
  lv_obj_set_style_local_text_color(label_time_hour, LV_LABEL_PART_MAIN, LV_STATE_DEFAULT, lv_color_hex(0x00FF00));

  label_time_minute = lv_label_create(lv_scr_act(), nullptr);
  lv_obj_align(label_time_minute, lv_scr_act(), LV_ALIGN_IN_TOP_RIGHT, 0, 0);
  lv_obj_set_style_local_text_color(label_time_minute, LV_LABEL_PART_MAIN, LV_STATE_DEFAULT, lv_color_hex(0x00FF00));

  label_time_second = lv_label_create(lv_scr_act(), nullptr);
  lv_obj_align(label_time_second, lv_scr_act(), LV_ALIGN_IN_BOTTOM_RIGHT, 0, 0);
  lv_obj_set_style_local_text_color(label_time_second, LV_LABEL_PART_MAIN, LV_STATE_DEFAULT, lv_color_hex(0x00FF00));

  stepValue = lv_label_create(lv_scr_act(), nullptr);
  lv_obj_set_style_local_text_color(stepValue, LV_LABEL_PART_MAIN, LV_STATE_DEFAULT, lv_color_hex(0x00FFE7));
  lv_label_set_text(stepValue, "0");
  lv_obj_align(stepValue, lv_scr_act(), LV_ALIGN_CENTER, 0, 30);

  stepIcon = lv_label_create(lv_scr_act(), nullptr);
  lv_obj_set_style_local_text_color(stepIcon, LV_LABEL_PART_MAIN, LV_STATE_DEFAULT, lv_color_hex(0x00FFE7));
  lv_label_set_text(stepIcon, Symbols::shoe);
  lv_obj_align(stepIcon, stepValue, LV_ALIGN_OUT_BOTTOM_MID, 0, 0);

  minute_body = lv_line_create(lv_scr_act(), NULL);
  minute_body_trace = lv_line_create(lv_scr_act(), NULL);
  hour_body = lv_line_create(lv_scr_act(), NULL);
  hour_body_trace = lv_line_create(lv_scr_act(), NULL);
  second_body = lv_line_create(lv_scr_act(), NULL);

  lv_style_init(&second_line_style);
  lv_style_set_line_width(&second_line_style, LV_STATE_DEFAULT, 3);
  lv_style_set_line_color(&second_line_style, LV_STATE_DEFAULT, LV_COLOR_RED);
  lv_style_set_line_rounded(&second_line_style, LV_STATE_DEFAULT, true);
  lv_obj_add_style(second_body, LV_LINE_PART_MAIN, &second_line_style);

  lv_style_init(&minute_line_style);
  lv_style_set_line_width(&minute_line_style, LV_STATE_DEFAULT, 7);
  lv_style_set_line_color(&minute_line_style, LV_STATE_DEFAULT, LV_COLOR_WHITE);
  lv_style_set_line_rounded(&minute_line_style, LV_STATE_DEFAULT, true);
  lv_obj_add_style(minute_body, LV_LINE_PART_MAIN, &minute_line_style);

  lv_style_init(&minute_line_style_trace);
  lv_style_set_line_width(&minute_line_style_trace, LV_STATE_DEFAULT, 3);
  lv_style_set_line_color(&minute_line_style_trace, LV_STATE_DEFAULT, LV_COLOR_WHITE);
  lv_style_set_line_rounded(&minute_line_style_trace, LV_STATE_DEFAULT, false);
  lv_obj_add_style(minute_body_trace, LV_LINE_PART_MAIN, &minute_line_style_trace);

  lv_style_init(&hour_line_style);
  lv_style_set_line_width(&hour_line_style, LV_STATE_DEFAULT, 7);
  lv_style_set_line_color(&hour_line_style, LV_STATE_DEFAULT, LV_COLOR_WHITE);
  lv_style_set_line_rounded(&hour_line_style, LV_STATE_DEFAULT, true);
  lv_obj_add_style(hour_body, LV_LINE_PART_MAIN, &hour_line_style);

  lv_style_init(&hour_line_style_trace);
  lv_style_set_line_width(&hour_line_style_trace, LV_STATE_DEFAULT, 3);
  lv_style_set_line_color(&hour_line_style_trace, LV_STATE_DEFAULT, LV_COLOR_WHITE);
  lv_style_set_line_rounded(&hour_line_style_trace, LV_STATE_DEFAULT, false);
  lv_obj_add_style(hour_body_trace, LV_LINE_PART_MAIN, &hour_line_style_trace);

  bleIcon = lv_label_create(lv_scr_act(), nullptr);
  lv_obj_set_style_local_text_color(bleIcon, LV_LABEL_PART_MAIN, LV_STATE_DEFAULT, lv_color_hex(0x0000FF));
  lv_label_set_text(bleIcon, Symbols::bluetooth);
  lv_obj_align(bleIcon, NULL, LV_ALIGN_CENTER, 0, 0);

  taskRefresh = lv_task_create(RefreshTaskCallback, LV_DISP_DEF_REFR_PERIOD, LV_TASK_PRIO_MID, this);
  UpdateClock();
}

WatchFaceAnalog::~WatchFaceAnalog() {
  lv_task_del(taskRefresh);

  lv_style_reset(&hour_line_style);
  lv_style_reset(&hour_line_style_trace);
  lv_style_reset(&minute_line_style);
  lv_style_reset(&minute_line_style_trace);
  lv_style_reset(&second_line_style);

  lv_obj_clean(lv_scr_act());
}

void WatchFaceAnalog::UpdateClock() {
  hour = dateTimeController.Hours();
  minute = dateTimeController.Minutes();
  second = dateTimeController.Seconds();
  char hoursChar[3];
  char minutesChar[3];
  char secondsChar[3];

  if (sMinute != minute) {
    auto const angle = minute * 6;
    minute_point[0] = CoordinateRelocate(30, angle);
    minute_point[1] = CoordinateRelocate(MinuteLength, angle);

    minute_point_trace[0] = CoordinateRelocate(5, angle);
    minute_point_trace[1] = CoordinateRelocate(31, angle);

    lv_line_set_points(minute_body, minute_point, 2);
    lv_line_set_points(minute_body_trace, minute_point_trace, 2);
  }

  if (sHour != hour || sMinute != minute) {
    sHour = hour;
    sMinute = minute;
    auto const angle = (hour * 30 + minute / 2);

    hour_point[0] = CoordinateRelocate(30, angle);
    hour_point[1] = CoordinateRelocate(HourLength, angle);

    hour_point_trace[0] = CoordinateRelocate(5, angle);
    hour_point_trace[1] = CoordinateRelocate(31, angle);

    lv_line_set_points(hour_body, hour_point, 2);
    lv_line_set_points(hour_body_trace, hour_point_trace, 2);
  }

  if (sSecond != second) {
    sSecond = second;
    auto const angle = second * 6;

    second_point[0] = CoordinateRelocate(-20, angle);
    second_point[1] = CoordinateRelocate(SecondLength, angle);
    lv_line_set_points(second_body, second_point, 2);
    //sprintf(hoursChar,"%02X", static_cast<int>(hour));
    //sprintf(minutesChar,"%02X", static_cast<int>(minute));
    if (settingsController.GetClockType() == Controllers::Settings::ClockType::H240X)
    {
     sprintf(hoursChar,"%02X", static_cast<int>(hour));
     lv_label_set_text_fmt(label_time_hour, "0X%s", hoursChar);

     sprintf(minutesChar,"%02X", static_cast<int>(minute));
     lv_label_set_text_fmt(label_time_minute, "0X%s", minutesChar);

     sprintf(secondsChar,"%02X", static_cast<int>(second));
     lv_label_set_text_fmt(label_time_second, "0X%s", secondsChar);
    } else if(settingsController.GetClockType() == Controllers::Settings::ClockType::H120X)
    {
     sprintf(hoursChar,"%02X", static_cast<int>(HourFormat12(hour)));
     lv_label_set_text_fmt(label_time_hour, "0X%s", hoursChar);

     sprintf(minutesChar,"%02X", static_cast<int>(minute));
     lv_label_set_text_fmt(label_time_minute, "0X%s", minutesChar);

     sprintf(secondsChar,"%02X", static_cast<int>(second));
     lv_label_set_text_fmt(label_time_second, "0X%s", secondsChar);
    } else if(settingsController.GetClockType() == Controllers::Settings::ClockType::H24)
    {
     sprintf(hoursChar,"%02d", static_cast<int>(hour));
     lv_label_set_text_fmt(label_time_hour, "%s", hoursChar);

     sprintf(minutesChar,"%02d", static_cast<int>(minute));
     lv_label_set_text_fmt(label_time_minute, "%s", minutesChar);

     sprintf(secondsChar,"%02d", static_cast<int>(second));
     lv_label_set_text_fmt(label_time_second, "%s", secondsChar);
    } else if(settingsController.GetClockType() == Controllers::Settings::ClockType::H12)
    {
     sprintf(hoursChar,"%02d", static_cast<int>(HourFormat12(hour)));
     lv_label_set_text_fmt(label_time_hour, "%s", hoursChar);

     sprintf(minutesChar,"%02d", static_cast<int>(minute));
     lv_label_set_text_fmt(label_time_minute, "%s", minutesChar);

     sprintf(secondsChar,"%02d", static_cast<int>(second));
     lv_label_set_text_fmt(label_time_second, "%s", secondsChar);
    }
  }
}

int WatchFaceAnalog::HourFormat12(int hour)
{
      if (hour == 0) {
        hour = 12;
      } else if (hour > 12) {
        hour -= 12;
      }
      return hour;
}


void WatchFaceAnalog::SetBatteryIcon() {
  auto batteryPercent = batteryPercentRemaining.Get();
  if (batteryPercent == 100) {
    lv_obj_set_style_local_text_color(batteryIcon, LV_LABEL_PART_MAIN, LV_STATE_DEFAULT, LV_COLOR_GREEN);
  } else {
    lv_obj_set_style_local_text_color(batteryIcon, LV_LABEL_PART_MAIN, LV_STATE_DEFAULT, LV_COLOR_WHITE);
  }
  lv_label_set_text(batteryIcon, BatteryIcon::GetBatteryIcon(batteryPercent));
}

void WatchFaceAnalog::Refresh() {
  isCharging = batteryController.IsCharging();
  if (isCharging.IsUpdated()) {
    if (isCharging.Get()) {
      lv_obj_set_style_local_text_color(batteryIcon, LV_LABEL_PART_MAIN, LV_STATE_DEFAULT, LV_COLOR_RED);
      lv_label_set_text(batteryIcon, Symbols::plug);
    } else {
      SetBatteryIcon();
    }
  }
  if (!isCharging.Get()) {
    batteryPercentRemaining = batteryController.PercentRemaining();
    if (batteryPercentRemaining.IsUpdated()) {
      SetBatteryIcon();
    }
  }

  notificationState = notificationManager.AreNewNotificationsAvailable();

  if (notificationState.IsUpdated()) {
    lv_label_set_text(notificationIcon, NotificationIcon::GetIcon(notificationState.Get()));
  }

  currentDateTime = dateTimeController.CurrentDateTime();

  if (currentDateTime.IsUpdated()) {
    Pinetime::Controllers::DateTime::Months month = dateTimeController.Month();
    uint8_t day = dateTimeController.Day();
    Pinetime::Controllers::DateTime::Days dayOfWeek = dateTimeController.DayOfWeek();

    UpdateClock();

    if ((month != currentMonth) || (dayOfWeek != currentDayOfWeek) || (day != currentDay)) {
      lv_label_set_text_fmt(label_date_day, "%s %02i", dateTimeController.DayOfWeekShortToString(), day);

      currentMonth = month;
      currentDayOfWeek = dayOfWeek;
      currentDay = day;
    }
  }

  stepCount = motionController.NbSteps();
  motionSensorOk = motionController.IsSensorOk();
  if (stepCount.IsUpdated() || motionSensorOk.IsUpdated()) {
    lv_label_set_text_fmt(stepValue, "%lu", stepCount.Get());
  }

}
