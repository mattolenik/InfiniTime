#include "displayapp/screens/WatchFaceSquircle.h"
#include <cmath>
#include <iostream>
#include <lvgl/lvgl.h>
#include "displayapp/screens/BatteryIcon.h"
#include "displayapp/screens/BleIcon.h"
#include "displayapp/screens/Symbols.h"
#include "displayapp/screens/NotificationIcon.h"
#include "components/settings/Settings.h"
#include "displayapp/InfiniTimeTheme.h"

using namespace Pinetime::Applications::Screens;

const float TAU = 6.28318;
const float PI_2 = 1.57079632679; // pi/2
const float HOUR_SLICE = TAU / 12.0;

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

  hour = 99;
  minute = 99;
  second = 99;

  backdrop = lv_obj_create(lv_scr_act(), nullptr);
  lv_obj_set_size(backdrop, 240, 240);
  lv_obj_align(backdrop, nullptr, LV_ALIGN_IN_TOP_LEFT, 0, 0);

  twelve = lv_label_create(lv_scr_act(), nullptr);
  lv_label_set_align(twelve, LV_LABEL_ALIGN_CENTER);
  lv_label_set_text_static(twelve, "12");
  lv_obj_set_pos(twelve, 110, 10);
  lv_obj_set_style_local_text_color(twelve, LV_LABEL_PART_MAIN, LV_STATE_DEFAULT, LV_COLOR_BLACK);

  batteryIcon.Create(lv_scr_act());
  lv_obj_align(batteryIcon.GetObject(), nullptr, LV_ALIGN_IN_TOP_RIGHT, -6, 6);

  plugIcon = lv_label_create(lv_scr_act(), nullptr);
  lv_label_set_text_static(plugIcon, Symbols::plug);
  lv_obj_align(plugIcon, nullptr, LV_ALIGN_IN_TOP_RIGHT, 0, 0);

  bleIcon = lv_label_create(lv_scr_act(), nullptr);
  lv_label_set_text_static(bleIcon, "");
  lv_obj_align(bleIcon, nullptr, LV_ALIGN_IN_TOP_RIGHT, -30, 6);

  notificationIcon = lv_label_create(lv_scr_act(), nullptr);
  lv_obj_set_style_local_text_color(notificationIcon, LV_LABEL_PART_MAIN, LV_STATE_DEFAULT, LV_COLOR_BLACK);
  lv_label_set_text_static(notificationIcon, NotificationIcon::GetIcon(false));
  lv_obj_align(notificationIcon, nullptr, LV_ALIGN_IN_TOP_LEFT, 0, 0);

  // Date - Day / Week day

  label_date_day = lv_label_create(lv_scr_act(), nullptr);
  lv_obj_set_style_local_text_color(label_date_day, LV_LABEL_PART_MAIN, LV_STATE_DEFAULT, color_date);
  lv_label_set_align(label_date_day, LV_LABEL_ALIGN_LEFT);
  lv_obj_align(label_date_day, nullptr, LV_ALIGN_CENTER, 45, 0);

  minute_body = lv_line_create(backdrop, nullptr);
  hour_body = lv_line_create(backdrop, nullptr);
  second_body = lv_line_create(backdrop, nullptr);

  lv_style_init(&second_line_style);
  lv_style_set_line_width(&second_line_style, LV_STATE_DEFAULT, width_second_hand);
  lv_style_set_line_color(&second_line_style, LV_STATE_DEFAULT, color_second_hand);
  lv_style_set_line_rounded(&second_line_style, LV_STATE_DEFAULT, false);
  lv_obj_add_style(second_body, LV_LINE_PART_MAIN, &second_line_style);

  lv_style_init(&minute_line_style);
  lv_style_set_line_width(&minute_line_style, LV_STATE_DEFAULT, width_minute_hand);
  lv_style_set_line_color(&minute_line_style, LV_STATE_DEFAULT, color_hour_minute_hands);
  lv_style_set_line_rounded(&minute_line_style, LV_STATE_DEFAULT, true);
  lv_obj_add_style(minute_body, LV_LINE_PART_MAIN, &minute_line_style);

  lv_style_init(&hour_line_style);
  lv_style_set_line_width(&hour_line_style, LV_STATE_DEFAULT, width_hour_hand);
  lv_style_set_line_color(&hour_line_style, LV_STATE_DEFAULT, color_hour_minute_hands);
  lv_style_set_line_rounded(&hour_line_style, LV_STATE_DEFAULT, true);
  lv_obj_add_style(hour_body, LV_LINE_PART_MAIN, &hour_line_style);

  lv_style_init(&hour_scale_style);
  lv_style_set_line_width(&hour_scale_style, LV_STATE_DEFAULT, width_hour_scales);
  lv_style_set_line_color(&hour_scale_style, LV_STATE_DEFAULT, color_hour_scales);
  lv_style_set_line_rounded(&hour_scale_style, LV_STATE_DEFAULT, false);

  lv_style_init(&backdrop_style);
  lv_style_set_bg_opa(&backdrop_style, LV_STATE_DEFAULT, LV_OPA_COVER);
  lv_style_set_bg_color(&backdrop_style, LV_STATE_DEFAULT, color_bg_grad_top);
  lv_style_set_bg_grad_color(&backdrop_style, LV_STATE_DEFAULT, color_bg_grad_bottom);
  lv_style_set_bg_grad_dir(&backdrop_style, LV_STATE_DEFAULT, LV_GRAD_DIR_VER);
  lv_style_set_bg_grad_stop(&backdrop_style, LV_STATE_DEFAULT, 239); // TODO: replace 239 with screenheight-1
  lv_style_set_clip_corner(&backdrop_style, LV_STATE_DEFAULT, true);
  lv_style_set_radius(&backdrop_style, LV_STATE_DEFAULT, 12);
  lv_obj_add_style(backdrop, LV_OBJ_PART_MAIN, &backdrop_style);

  for (int i = 0; i < 12; i++) {
    hour_scale_line_objs[i] = lv_line_create(lv_scr_act(), nullptr);
    lv_obj_add_style(hour_scale_line_objs[i], LV_LINE_PART_MAIN, &hour_scale_style);
  }

  lv_disp_t* disp = lv_disp_get_default();
  lv_coord_t disp_width = lv_disp_get_hor_res(disp);
  lv_coord_t disp_height = lv_disp_get_ver_res(disp);
  float maxRadius = fmin(disp_width, disp_height) / 2;
  CalculateSquircleRadii(hour_scale_line_objs, maxRadius * 0.97, 2.4, 1, 1);

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

/**
 * Rounds x,y coordinates that are floats to the nearest integers and places them where pointed as an lv_point_t
 */
void WatchFaceSquircle::NearestPoint(float x, float y, lv_point_t* point) {
  *point = {static_cast<lv_coord_t>(nearbyintf(x)), static_cast<lv_coord_t>(nearbyintf(y))};
}

template <size_t N>
void WatchFaceSquircle::CalculateSquircleRadii(lv_obj_t* (&line_objs)[N], float size, float n, float a, float b) {
  float inverse_n = -1.0 / n;
  for (size_t i = 0; i < N; i++) {
    float theta = (static_cast<float>(i) / static_cast<float>(N)) * TAU;
    float cos_t = cosf(theta);
    float sin_t = sinf(theta);
    // The superellipse formula gives the radius for use in a polar coordinate, name it (r, theta)
    // Theta is already known, use the formula to get the radius:
    float r1 = powf(powf(fabs(cos_t / a), n) + powf(fabs(sin_t / a), n), inverse_n) * size;
    float r2 = r1 * 0.95;
    NearestPoint(r1 * cosf(theta) + 120, r1 * sinf(theta) + 120, &scales[i].points[0]);
    NearestPoint(r2 * cosf(theta) + 120, r2 * sinf(theta) + 120, &scales[i].points[1]);
    lv_line_set_points(line_objs[i], scales[i].points, 2);
  }
}

void WatchFaceSquircle::UpdateClock() {
  uint8_t latest_hour = dateTimeController.Hours();
  uint8_t latest_minute = dateTimeController.Minutes();
  uint8_t latest_second = dateTimeController.Seconds();
  lv_coord_t offset = 120; // TODO: replace with screenwidth/2
  float r, t, cos_t, sin_t;

  if (latest_minute != minute) {
    minute = latest_minute;
    minutef = static_cast<float>(minute);

    t = ((minutef / 60) * TAU) - PI_2;
    cos_t = cosf(t);
    sin_t = sinf(t);

    NearestPoint(length_minute_hand * cos_t + offset, length_minute_hand * sin_t + offset, &minute_point[0]);
    minute_point[1] = {offset, offset};

    lv_line_set_points(minute_body, minute_point, 2);
  }

  if (latest_hour != hour || latest_minute != minute) {
    hour = latest_hour;
    hourf = static_cast<float>(hour);
    twelveHour = hour % 12;
    twelveHourf = static_cast<float>(twelveHour);

    t = (twelveHourf * HOUR_SLICE - PI_2) + (minutef / 60 * HOUR_SLICE);
    cos_t = cosf(t);
    sin_t = sinf(t);

    NearestPoint(length_hour_hand * cos_t + offset, length_hour_hand * sin_t + offset, &hour_point[0]);
    hour_point[1] = {offset, offset};

    lv_line_set_points(hour_body, hour_point, 2);
  }

  if (latest_second != second) {
    second = latest_second;
    secondf = static_cast<float>(second);

    t = ((secondf / 60) * TAU) - PI_2;
    cos_t = cosf(t);
    sin_t = sinf(t);

    NearestPoint(length_second_hand * cos_t + offset, length_second_hand * sin_t + offset, &second_point[0]);
    NearestPoint(length_second_hand_back * cos_t + offset, length_second_hand_back * sin_t + offset, &second_point[1]);

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
      lv_label_set_text_fmt(label_date_day, "%s %02i", dateTimeController.DayOfWeekShortToString(), dateTimeController.Day());
    }
  }
}
