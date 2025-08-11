#include "displayapp/screens/WatchFaceAnalogCustom.h"
#include <cmath>
#include <limits>
#include <lvgl/lvgl.h>
#include "components/heartrate/HeartRateController.h"
#include "components/motion/MotionController.h"
#include "components/settings/Settings.h"
#include "displayapp/InfiniTimeTheme.h"
#include "displayapp/screens/BatteryIcon.h"
#include "displayapp/screens/BleIcon.h"
#include "displayapp/screens/NotificationIcon.h"
#include "displayapp/screens/Symbols.h"

using namespace Pinetime::Applications::Screens;

namespace {
  constexpr int16_t HourLength = 70;
  constexpr int16_t MinuteLength = 90;
  constexpr int16_t SecondLength = 110;

  static auto color_nordic_ash = lv_color_hex(0x697D97);
  static auto color_nordic_white = lv_color_hex(0xdde2e3);
  static auto color_nordic_gray = lv_color_hex(0x9aacb8);
  static auto color_nordic_orange = lv_color_hex(0xb37c57);
  static auto color_nordic_green = lv_color_hex(0xa3be8c);
  static auto color_nordic_blue = lv_color_hex(0x88c0d0);

  // LVGL sin isn't constexpr (though it could be if it were C++) so fix size here
  // All the types are hardcoded anyway and would need changing if the size changed
  // sin(90) = 1 so the value of _lv_trigo_sin(90) is the scaling factor
  constexpr int16_t LV_TRIG_SCALE = std::numeric_limits<int16_t>::max(); // = _lv_trigo_sin(90)

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
    return lv_point_t {.x = CoordinateXRelocate(radius * static_cast<int32_t>(Sine(angle)) / LV_TRIG_SCALE),
                       .y = CoordinateYRelocate(radius * static_cast<int32_t>(Cosine(angle)) / LV_TRIG_SCALE)};
  }

}

WatchFaceAnalogCustom::WatchFaceAnalogCustom(Controllers::DateTime& dateTimeController,
                                 const Controllers::Battery& batteryController,
                                 const Controllers::Ble& bleController,
                                 Controllers::NotificationManager& notificationManager,
                                 Controllers::Settings& settingsController,
                                 Controllers::HeartRateController& heartRateController,
                                 Controllers::MotionController& motionController)
  : currentDateTime {{}},
    batteryIcon(false),
    dateTimeController {dateTimeController},
    batteryController {batteryController},
    bleController {bleController},
    notificationManager {notificationManager},
    settingsController {settingsController},
    heartRateController {heartRateController},
    motionController {motionController} {

  sHour = 99;
  sMinute = 99;
  sSecond = 99;

  minor_scales = lv_linemeter_create(lv_scr_act(), nullptr);
  lv_linemeter_set_scale(minor_scales, 360, 61);
  // lv_linemeter_set_angle_offset(minor_scales, 180);
  lv_obj_set_size(minor_scales, 240, 240);
  lv_obj_align(minor_scales, nullptr, LV_ALIGN_CENTER, 0, 0);
  lv_obj_set_style_local_bg_opa(minor_scales, LV_LINEMETER_PART_MAIN, LV_STATE_DEFAULT, LV_OPA_TRANSP);
  lv_obj_set_style_local_scale_width(minor_scales, LV_LINEMETER_PART_MAIN, LV_STATE_DEFAULT, 4);
  lv_obj_set_style_local_scale_end_line_width(minor_scales, LV_LINEMETER_PART_MAIN, LV_STATE_DEFAULT, 1);
  lv_obj_set_style_local_scale_end_color(minor_scales, LV_LINEMETER_PART_MAIN, LV_STATE_DEFAULT, color_nordic_gray);

  major_scales = lv_linemeter_create(lv_scr_act(), nullptr);
  lv_linemeter_set_scale(major_scales, 300, 11);
  lv_linemeter_set_angle_offset(major_scales, 180);
  lv_obj_set_size(major_scales, 240, 240);
  lv_obj_align(major_scales, nullptr, LV_ALIGN_CENTER, 0, 0);
  lv_obj_set_style_local_bg_opa(major_scales, LV_LINEMETER_PART_MAIN, LV_STATE_DEFAULT, LV_OPA_TRANSP);
  lv_obj_set_style_local_scale_width(major_scales, LV_LINEMETER_PART_MAIN, LV_STATE_DEFAULT, 6);
  lv_obj_set_style_local_scale_end_line_width(major_scales, LV_LINEMETER_PART_MAIN, LV_STATE_DEFAULT, 4);
  lv_obj_set_style_local_scale_end_color(major_scales, LV_LINEMETER_PART_MAIN, LV_STATE_DEFAULT, color_nordic_gray);

  large_scales = lv_linemeter_create(lv_scr_act(), nullptr);
  lv_linemeter_set_scale(large_scales, 270, 4);
  lv_linemeter_set_angle_offset(large_scales, 45);
  lv_obj_set_size(large_scales, 240, 240);
  lv_obj_align(large_scales, nullptr, LV_ALIGN_CENTER, 0, 0);
  lv_obj_set_style_local_bg_opa(large_scales, LV_LINEMETER_PART_MAIN, LV_STATE_DEFAULT, LV_OPA_TRANSP);
  lv_obj_set_style_local_scale_width(large_scales, LV_LINEMETER_PART_MAIN, LV_STATE_DEFAULT, 20);
  lv_obj_set_style_local_scale_end_line_width(large_scales, LV_LINEMETER_PART_MAIN, LV_STATE_DEFAULT, 4);
  lv_obj_set_style_local_scale_end_color(large_scales, LV_LINEMETER_PART_MAIN, LV_STATE_DEFAULT, color_nordic_ash);

  // Upper Info ZOne
  batteryIcon.Create(lv_scr_act());
  batteryIcon.SetColor(color_nordic_green);
  lv_obj_align(batteryIcon.GetObject(), nullptr, LV_ALIGN_IN_TOP_RIGHT, 0, 0);

  plugIcon = lv_label_create(lv_scr_act(), nullptr);
  lv_label_set_text_static(plugIcon, Symbols::plug);
  lv_obj_set_style_local_scale_end_color(plugIcon, LV_LABEL_PART_MAIN, LV_STATE_DEFAULT, color_nordic_ash);
  lv_obj_align(plugIcon, nullptr, LV_ALIGN_IN_TOP_RIGHT, 0, 0);

  bleIcon = lv_label_create(lv_scr_act(), nullptr);
  lv_label_set_text_static(bleIcon, "");
  lv_obj_set_style_local_scale_end_color(bleIcon, LV_LABEL_PART_MAIN, LV_STATE_DEFAULT, color_nordic_blue);
  lv_obj_align(bleIcon, nullptr, LV_ALIGN_IN_TOP_RIGHT, -30, 0);

  notificationIcon = lv_label_create(lv_scr_act(), nullptr);
  lv_obj_set_style_local_text_color(notificationIcon, LV_LABEL_PART_MAIN, LV_STATE_DEFAULT, color_nordic_orange);
  lv_label_set_text_static(notificationIcon, NotificationIcon::GetIcon(false));
  lv_obj_align(notificationIcon, nullptr, LV_ALIGN_IN_TOP_LEFT, 0, 0);

  // Date - Day / Week day
  uint16_t year = dateTimeController.Year();
  auto month = static_cast<uint8_t>(dateTimeController.Month());
  uint8_t day = dateTimeController.Day();

  label_year = lv_label_create(lv_scr_act(), nullptr);
  lv_obj_set_style_local_text_color(label_year, LV_LABEL_PART_MAIN, LV_STATE_DEFAULT, color_nordic_ash);
  lv_label_set_text_fmt(label_year, "%d", year);
  lv_label_set_align(label_year, LV_LABEL_ALIGN_CENTER);
  lv_obj_align(label_year, nullptr, LV_ALIGN_CENTER, -3, -50);

  label_date_day = lv_label_create(lv_scr_act(), nullptr);
  lv_obj_set_style_local_text_color(label_date_day, LV_LABEL_PART_MAIN, LV_STATE_DEFAULT, color_nordic_ash);
  lv_label_set_text_fmt(label_date_day, "%02i/%02i", month, day);
  lv_label_set_align(label_date_day, LV_LABEL_ALIGN_CENTER);
  lv_obj_align(label_date_day, nullptr, LV_ALIGN_CENTER, -50, 0);

  label_week = lv_label_create(lv_scr_act(), nullptr);
  lv_obj_set_style_local_text_color(label_week, LV_LABEL_PART_MAIN, LV_STATE_DEFAULT, color_nordic_ash);
  lv_label_set_text_fmt(label_week, "%s", dateTimeController.DayOfWeekShortToString());
  lv_label_set_align(label_week, LV_LABEL_ALIGN_CENTER);
  lv_obj_align(label_week, nullptr, LV_ALIGN_CENTER, 50, 0);

  label_time = lv_label_create(lv_scr_act(), nullptr);
  lv_obj_set_style_local_text_color(label_time, LV_LABEL_PART_MAIN, LV_STATE_DEFAULT, color_nordic_ash);
  lv_obj_align(label_time, lv_scr_act(), LV_ALIGN_CENTER, -25, 50);

  // Bottom Info Zone
  heartbeatIcon = lv_label_create(lv_scr_act(), nullptr);
  lv_label_set_text_static(heartbeatIcon, Symbols::heartBeat);
  lv_obj_set_style_local_text_color(heartbeatIcon, LV_LABEL_PART_MAIN, LV_STATE_DEFAULT, color_nordic_orange);
  lv_obj_align(heartbeatIcon, lv_scr_act(), LV_ALIGN_IN_BOTTOM_LEFT, 0, 0);

  heartbeatValue = lv_label_create(lv_scr_act(), nullptr);
  lv_obj_set_style_local_text_color(heartbeatValue, LV_LABEL_PART_MAIN, LV_STATE_DEFAULT, color_nordic_ash);
  lv_label_set_text_static(heartbeatValue, "-");
  lv_obj_align(heartbeatValue, heartbeatIcon, LV_ALIGN_OUT_RIGHT_MID, 5, 0);

  stepValue = lv_label_create(lv_scr_act(), nullptr);
  lv_obj_set_style_local_text_color(stepValue, LV_LABEL_PART_MAIN, LV_STATE_DEFAULT, color_nordic_ash);
  lv_label_set_text_static(stepValue, "-");
  lv_obj_align(stepValue, lv_scr_act(), LV_ALIGN_IN_BOTTOM_RIGHT, 0, 0);

  stepIcon = lv_label_create(lv_scr_act(), nullptr);
  lv_obj_set_style_local_text_color(stepIcon, LV_LABEL_PART_MAIN, LV_STATE_DEFAULT, color_nordic_blue);
  lv_label_set_text_static(stepIcon, Symbols::shoe);
  lv_obj_align(stepIcon, stepValue, LV_ALIGN_OUT_LEFT_MID, -5, 0);

  minute_body = lv_line_create(lv_scr_act(), nullptr);
  minute_body_trace = lv_line_create(lv_scr_act(), nullptr);
  hour_body = lv_line_create(lv_scr_act(), nullptr);
  hour_body_trace = lv_line_create(lv_scr_act(), nullptr);
  second_body = lv_line_create(lv_scr_act(), nullptr);

  lv_style_init(&second_line_style);
  lv_style_set_line_width(&second_line_style, LV_STATE_DEFAULT, 3);
  lv_style_set_line_color(&second_line_style, LV_STATE_DEFAULT, color_nordic_orange);
  lv_style_set_line_rounded(&second_line_style, LV_STATE_DEFAULT, true);
  lv_obj_add_style(second_body, LV_LINE_PART_MAIN, &second_line_style);

  lv_style_init(&minute_line_style);
  lv_style_set_line_width(&minute_line_style, LV_STATE_DEFAULT, 7);
  lv_style_set_line_color(&minute_line_style, LV_STATE_DEFAULT, color_nordic_white);
  lv_style_set_line_rounded(&minute_line_style, LV_STATE_DEFAULT, true);
  lv_obj_add_style(minute_body, LV_LINE_PART_MAIN, &minute_line_style);

  lv_style_init(&minute_line_style_trace);
  lv_style_set_line_width(&minute_line_style_trace, LV_STATE_DEFAULT, 3);
  lv_style_set_line_color(&minute_line_style_trace, LV_STATE_DEFAULT, color_nordic_white);
  lv_style_set_line_rounded(&minute_line_style_trace, LV_STATE_DEFAULT, false);
  lv_obj_add_style(minute_body_trace, LV_LINE_PART_MAIN, &minute_line_style_trace);

  lv_style_init(&hour_line_style);
  lv_style_set_line_width(&hour_line_style, LV_STATE_DEFAULT, 7);
  lv_style_set_line_color(&hour_line_style, LV_STATE_DEFAULT, color_nordic_white);
  lv_style_set_line_rounded(&hour_line_style, LV_STATE_DEFAULT, true);
  lv_obj_add_style(hour_body, LV_LINE_PART_MAIN, &hour_line_style);

  lv_style_init(&hour_line_style_trace);
  lv_style_set_line_width(&hour_line_style_trace, LV_STATE_DEFAULT, 3);
  lv_style_set_line_color(&hour_line_style_trace, LV_STATE_DEFAULT, color_nordic_white);
  lv_style_set_line_rounded(&hour_line_style_trace, LV_STATE_DEFAULT, false);
  lv_obj_add_style(hour_body_trace, LV_LINE_PART_MAIN, &hour_line_style_trace);

  taskRefresh = lv_task_create(RefreshTaskCallback, LV_DISP_DEF_REFR_PERIOD, LV_TASK_PRIO_MID, this);

  Refresh();
}

WatchFaceAnalogCustom::~WatchFaceAnalogCustom() {
  lv_task_del(taskRefresh);

  lv_style_reset(&hour_line_style);
  lv_style_reset(&hour_line_style_trace);
  lv_style_reset(&minute_line_style);
  lv_style_reset(&minute_line_style_trace);
  lv_style_reset(&second_line_style);

  lv_obj_clean(lv_scr_act());
}

void WatchFaceAnalogCustom::UpdateClock() {
  uint8_t hour = dateTimeController.Hours();
  uint8_t minute = dateTimeController.Minutes();
  uint8_t second = dateTimeController.Seconds();

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
  }

  lv_label_set_text_fmt(label_time, "%02d:%02d:%02d", hour, minute, second);
}

void WatchFaceAnalogCustom::SetBatteryIcon() {
  auto batteryPercent = batteryPercentRemaining.Get();
  batteryIcon.SetColor(color_nordic_green);
  batteryIcon.SetBatteryPercentage(batteryPercent);
}

void WatchFaceAnalogCustom::Refresh() {
  isCharging = batteryController.IsCharging();
  if (isCharging.IsUpdated()) {
    if (isCharging.Get()) {
      lv_obj_set_hidden(batteryIcon.GetObject(), true);
      lv_obj_set_hidden(plugIcon, false);
    } else {
      lv_obj_set_hidden(batteryIcon.GetObject(), false);
      lv_obj_set_hidden(plugIcon, true);
      SetBatteryIcon();
    }
  }
  if (!isCharging.Get()) {
    batteryPercentRemaining = batteryController.PercentRemaining();
    if (batteryPercentRemaining.IsUpdated()) {
      SetBatteryIcon();
    }
  }

  bleState = bleController.IsConnected();
  if (bleState.IsUpdated()) {
    lv_obj_set_style_local_text_color(bleIcon, LV_LABEL_PART_MAIN, LV_STATE_DEFAULT, color_nordic_blue);
    if (bleState.Get()) {
      lv_label_set_text_static(bleIcon, Symbols::bluetooth);
    } else {
      lv_label_set_text_static(bleIcon, "");
    }
  }

  notificationState = notificationManager.AreNewNotificationsAvailable();
  if (notificationState.IsUpdated()) {
    lv_obj_set_style_local_text_color(notificationIcon, LV_LABEL_PART_MAIN, LV_STATE_DEFAULT, color_nordic_ash);
    lv_label_set_text_static(notificationIcon, NotificationIcon::GetIcon(notificationState.Get()));
  }

  currentDateTime = dateTimeController.CurrentDateTime();
  if (currentDateTime.IsUpdated()) {
    UpdateClock();

    currentDate = std::chrono::time_point_cast<std::chrono::days>(currentDateTime.Get());
    if (currentDate.IsUpdated()) {
      uint16_t year = dateTimeController.Year();
      auto month = static_cast<uint8_t>(dateTimeController.Month());
      uint8_t day = dateTimeController.Day();
      lv_label_set_text_fmt(label_year, "%d", year);
      lv_label_set_text_fmt(label_date_day, "%02i/%02i", month, day);
      lv_label_set_text_fmt(label_week, "%s", dateTimeController.DayOfWeekShortToString());
    }
  }

  heartbeat = heartRateController.HeartRate();
  heartbeatRunning = heartRateController.State() != Controllers::HeartRateController::States::Stopped;
  if (heartbeat.IsUpdated() || heartbeatRunning.IsUpdated()) {
    lv_obj_set_style_local_text_color(heartbeatIcon, LV_LABEL_PART_MAIN, LV_STATE_DEFAULT, color_nordic_orange);
    if (heartbeatRunning.Get()) {
      lv_label_set_text_fmt(heartbeatValue, "%d", heartbeat.Get());
    } else {
      lv_label_set_text_static(heartbeatValue, "-");
    }

    lv_obj_realign(heartbeatIcon);
    lv_obj_realign(heartbeatValue);
  }

  stepCount = motionController.NbSteps();
  if (stepCount.IsUpdated()) {
    lv_label_set_text_fmt(stepValue, "%lu", stepCount.Get());
    lv_obj_realign(stepValue);
    lv_obj_realign(stepIcon);
  }
}
