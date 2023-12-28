#include "displayapp/screens/WatchFaceSquircle.h"
#include <cmath>
#include <lvgl/lvgl.h>
#include "displayapp/screens/BatteryIcon.h"
#include "displayapp/screens/BleIcon.h"
#include "displayapp/screens/Symbols.h"
#include "displayapp/screens/NotificationIcon.h"
#include "components/settings/Settings.h"
#include "displayapp/InfiniTimeTheme.h"

using namespace Pinetime::Applications::Screens;

namespace {
  // sin(90) = 1 so the value of _lv_trigo_sin(90) is the scaling factor
  const auto LV_TRIG_SCALE = _lv_trigo_sin(90);

  const float TAU = 2 * M_PI;

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

WatchFaceSquircle::WatchFaceSquircle(Controllers::DateTime& dateTimeController,
                                     const Controllers::Battery& batteryController,
                                     const Controllers::Ble& bleController,
                                     Controllers::NotificationManager& notificationManager,
                                     Controllers::Settings& settingsController)
  : currentDateTime {{}},
    batteryIcon(true),
    dateTimeController {dateTimeController},
    batteryController {batteryController},
    bleController {bleController},
    notificationManager {notificationManager},
    settingsController {settingsController} {

  sHour = 99;
  sMinute = 99;
  sSecond = 99;

  twelve = lv_label_create(lv_scr_act(), nullptr);
  lv_label_set_align(twelve, LV_LABEL_ALIGN_CENTER);
  lv_label_set_text_static(twelve, "12");
  lv_obj_set_pos(twelve, 110, 10);
  lv_obj_set_style_local_text_color(twelve, LV_LABEL_PART_MAIN, LV_STATE_DEFAULT, LV_COLOR_AQUA);

  batteryIcon.Create(lv_scr_act());
  lv_obj_align(batteryIcon.GetObject(), nullptr, LV_ALIGN_IN_TOP_RIGHT, 0, 0);

  plugIcon = lv_label_create(lv_scr_act(), nullptr);
  lv_label_set_text_static(plugIcon, Symbols::plug);
  lv_obj_align(plugIcon, nullptr, LV_ALIGN_IN_TOP_RIGHT, 0, 0);

  bleIcon = lv_label_create(lv_scr_act(), nullptr);
  lv_label_set_text_static(bleIcon, "");
  lv_obj_align(bleIcon, nullptr, LV_ALIGN_IN_TOP_RIGHT, -30, 0);

  notificationIcon = lv_label_create(lv_scr_act(), nullptr);
  lv_obj_set_style_local_text_color(notificationIcon, LV_LABEL_PART_MAIN, LV_STATE_DEFAULT, LV_COLOR_LIME);
  lv_label_set_text_static(notificationIcon, NotificationIcon::GetIcon(false));
  lv_obj_align(notificationIcon, nullptr, LV_ALIGN_IN_TOP_LEFT, 0, 0);

  // Date - Day / Week day

  label_date_day = lv_label_create(lv_scr_act(), nullptr);
  lv_obj_set_style_local_text_color(label_date_day, LV_LABEL_PART_MAIN, LV_STATE_DEFAULT, Colors::orange);
  lv_label_set_text_fmt(label_date_day, "%s\n%02i", dateTimeController.DayOfWeekShortToString(), dateTimeController.Day());
  lv_label_set_align(label_date_day, LV_LABEL_ALIGN_CENTER);
  lv_obj_align(label_date_day, nullptr, LV_ALIGN_CENTER, 50, 0);

  minute_body = lv_line_create(lv_scr_act(), nullptr);
  hour_body = lv_line_create(lv_scr_act(), nullptr);
  second_body = lv_line_create(lv_scr_act(), nullptr);

  lv_style_init(&second_line_style);
  lv_style_set_line_width(&second_line_style, LV_STATE_DEFAULT, 3);
  lv_style_set_line_color(&second_line_style, LV_STATE_DEFAULT, LV_COLOR_BLUE);
  lv_style_set_line_rounded(&second_line_style, LV_STATE_DEFAULT, true);
  lv_obj_add_style(second_body, LV_LINE_PART_MAIN, &second_line_style);

  lv_style_init(&minute_line_style);
  lv_style_set_line_width(&minute_line_style, LV_STATE_DEFAULT, 3);
  lv_style_set_line_color(&minute_line_style, LV_STATE_DEFAULT, LV_COLOR_WHITE);
  lv_style_set_line_rounded(&minute_line_style, LV_STATE_DEFAULT, false);
  lv_obj_add_style(minute_body, LV_LINE_PART_MAIN, &minute_line_style);

  lv_style_init(&hour_line_style);
  lv_style_set_line_width(&hour_line_style, LV_STATE_DEFAULT, 3);
  lv_style_set_line_color(&hour_line_style, LV_STATE_DEFAULT, LV_COLOR_WHITE);
  lv_style_set_line_rounded(&hour_line_style, LV_STATE_DEFAULT, false);
  lv_obj_add_style(hour_body, LV_LINE_PART_MAIN, &hour_line_style);

  lv_style_init(&hour_scale_style);
  lv_style_set_line_width(&hour_scale_style, LV_STATE_DEFAULT, 3);
  lv_style_set_line_color(&hour_scale_style, LV_STATE_DEFAULT, LV_COLOR_WHITE);
  lv_style_set_line_rounded(&hour_scale_style, LV_STATE_DEFAULT, false);

  for (int i = 0; i < 60; i++) {
    hour_scale_line_objs[i] = lv_line_create(lv_scr_act(), nullptr);
    lv_obj_add_style(hour_scale_line_objs[i], LV_LINE_PART_MAIN, &hour_scale_style);
  }

  lv_disp_t* disp = lv_disp_get_default();
  lv_coord_t disp_width = lv_disp_get_hor_res(disp);
  lv_coord_t disp_height = lv_disp_get_ver_res(disp);
  float maxRadius = fmin(disp_width, disp_height) / 2;
  CalculateSquircleRadii(scale_radii, maxRadius * 0.9, 3, 1, 1);

  taskRefresh = lv_task_create(RefreshTaskCallback, LV_DISP_DEF_REFR_PERIOD, LV_TASK_PRIO_MID, this);

  Refresh();
}

WatchFaceSquircle::~WatchFaceSquircle() {
  lv_task_del(taskRefresh);

  lv_style_reset(&hour_line_style);
  lv_style_reset(&minute_line_style);
  lv_style_reset(&second_line_style);
  lv_style_reset(&hour_scale_style);

  lv_obj_clean(lv_scr_act());
}

template <size_t N>
void WatchFaceSquircle::DrawScales(float (&radii)[N], float length_scale, int every_nth) {
  // for (int i = 0; i < N; i += every_nth) {
  //   float r1 = radii[i];
  //   float r2 = r1 * length_scale;
  //   float theta = (static_cast<float>(i) / static_cast<float>(N)) * TAU;
  //   NearestPoint(r1 * cosf(theta) + 120, r1 * sinf(theta) + 120, &scales[i].points[0]);
  //   NearestPoint(r2 * cosf(theta) + 120, r2 * sinf(theta) + 120, &scales[i].points[1]);
  //   lv_line_set_points(hour_scale_line_objs[i], scales[i].points, 2);
  // }
}

/**
 * Rounds x,y coordinates that are floats to the nearest integers and places them where pointed as an lv_point_t
 */
void WatchFaceSquircle::NearestPoint(float x, float y, lv_point_t* point) {
  *point = {static_cast<lv_coord_t>(nearbyintf(x)), static_cast<lv_coord_t>(nearbyintf(y))};
}

template <size_t N>
void WatchFaceSquircle::CalculateSquircleRadii(float (&radii)[N], float size, float n, float a, float b) {
  float inverse_n = -1.0 / n;
  for (int i = 0; i < N; i++) {
    float theta = (static_cast<float>(i) / static_cast<float>(N)) * TAU;
    float cos_t = cosf(theta);
    float sin_t = sinf(theta);
    // The superellipse formula gives the radius for use in a polar coordinate, name it (r, theta)
    // Theta is already known, use the formula to get the radius:
    radii[i] = powf(powf(fabs(cos_t / a), n) + powf(fabs(sin_t / a), n), inverse_n) * size; // scale by caller's size
    float r1 = radii[i];
    float r2 = r1 * 0.9;
    NearestPoint(r1 * cosf(theta) + 120, r1 * sinf(theta) + 120, &scales[i].points[0]);
    NearestPoint(r2 * cosf(theta) + 120, r2 * sinf(theta) + 120, &scales[i].points[1]);
    lv_line_set_points(hour_scale_line_objs[i], scales[i].points, 2);
  }
}

void WatchFaceSquircle::UpdateClock() {
  DrawScales(scale_radii, 0.95, 15);

  uint8_t hour = dateTimeController.Hours();
  uint8_t minute = dateTimeController.Minutes();
  uint8_t second = dateTimeController.Seconds();

  if (sMinute != minute) {
    auto const angle = minute * 6;
    minute_point[0] = CoordinateRelocate(5, angle);
    minute_point[1] = CoordinateRelocate(31, angle);

    lv_line_set_points(minute_body, minute_point, 2);
  }

  if (sHour != hour || sMinute != minute) {
    sHour = hour;
    sMinute = minute;
    auto const angle = (hour * 30 + minute / 2);

    hour_point[0] = CoordinateRelocate(5, angle);
    hour_point[1] = CoordinateRelocate(31, angle);

    lv_line_set_points(hour_body, hour_point, 2);
  }

  if (sSecond != second) {
    sSecond = second;
    float r1 = scale_radii[second];
    float r2 = r1 - 20;
    float theta = (static_cast<float>(second) / 60.0) * TAU;
    float cos_t = cosf(theta);
    float sin_t = sinf(theta);
    NearestPoint(r1 * cos_t + 120, r1 * sin_t + 120, &second_point[0]);
    NearestPoint(r2 * cos_t + 120, r2 * sin_t + 120, &second_point[1]);
    lv_line_set_points(second_body, second_point, 2);
  }
}

void WatchFaceSquircle::SetBatteryIcon() {
  auto batteryPercent = batteryPercentRemaining.Get();
  batteryIcon.SetBatteryPercentage(batteryPercent);
}

void WatchFaceSquircle::Refresh() {
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
    if (bleState.Get()) {
      lv_label_set_text_static(bleIcon, Symbols::bluetooth);
    } else {
      lv_label_set_text_static(bleIcon, "");
    }
  }

  notificationState = notificationManager.AreNewNotificationsAvailable();

  if (notificationState.IsUpdated()) {
    lv_label_set_text_static(notificationIcon, NotificationIcon::GetIcon(notificationState.Get()));
  }

  currentDateTime = dateTimeController.CurrentDateTime();
  if (currentDateTime.IsUpdated()) {
    UpdateClock();

    currentDate = std::chrono::time_point_cast<days>(currentDateTime.Get());
    if (currentDate.IsUpdated()) {
      lv_label_set_text_fmt(label_date_day, "%s\n%02i", dateTimeController.DayOfWeekShortToString(), dateTimeController.Day());
    }
  }
}
