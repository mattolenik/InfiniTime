#include "displayapp/screens/WatchFaceSquircle.h"
#include <cmath>
#include <lvgl/lvgl.h>
#include "displayapp/screens/BatteryIcon.h"
#include "displayapp/screens/BleIcon.h"
#include "displayapp/screens/Symbols.h"
#include "displayapp/screens/NotificationIcon.h"
#include "components/heartrate/HeartRateController.h"
#include "components/settings/Settings.h"
#include "displayapp/InfiniTimeTheme.h"

using namespace Pinetime::Applications::Screens;

const float TAU = 6.28318;
const float PI_2 = 1.57079632679; // pi/2
const float PI_4 = 0.78539816339; // pi/4
const float HOUR_SLICE = TAU / 12.0;

WatchFaceSquircle::WatchFaceSquircle(Controllers::DateTime& dateTimeController,
                                     const Controllers::Battery& batteryController,
                                     const Controllers::Ble& bleController,
                                     Controllers::NotificationManager& notificationManager,
                                     Controllers::HeartRateController& heartRateController,
                                     Controllers::Settings& settingsController)
  : currentDateTime {{}},
    dateTimeController {dateTimeController},
    batteryController {batteryController},
    bleController {bleController},
    notificationManager {notificationManager},
    heartRateController {heartRateController},
    settingsController {settingsController},
    batteryIcon(true),
    display {lv_disp_get_default()} {

  hour = 99;
  minute = 99;
  second = 99;

  lv_coord_t disp_width = lv_disp_get_hor_res(display);
  lv_coord_t disp_height = lv_disp_get_ver_res(display);
  center_x = disp_width / 2;
  center_y = disp_height / 2;
  screen_area = {0, 0, 239, 239};

  backdrop_gradient[0] = lv_color_hex(0x5B96BE);
  backdrop_gradient[1] = lv_color_hex(0xFCFAEF);

  backdrop = lv_obj_create(lv_scr_act(), nullptr);
  lv_obj_set_size(backdrop, disp_width, disp_height);
  lv_obj_set_pos(backdrop, 0, 0);

  time_box = lv_obj_create(lv_scr_act(), nullptr);
  lv_obj_set_size(time_box, 75, 26);
  lv_obj_align(time_box, nullptr, LV_ALIGN_IN_LEFT_MID, 5, 0);

  label_time = lv_label_create(time_box, nullptr);
  lv_obj_set_style_local_text_color(label_time, LV_LABEL_PART_MAIN, LV_STATE_DEFAULT, lv_color_hex(0xFDF8DC));
  lv_label_set_align(label_time, LV_LABEL_ALIGN_CENTER);
  lv_obj_align(label_time, nullptr, LV_ALIGN_IN_LEFT_MID, 7, -1);

  twelve = lv_label_create(lv_scr_act(), nullptr);
  lv_label_set_align(twelve, LV_LABEL_ALIGN_CENTER);
  lv_label_set_text_static(twelve, "12");
  lv_obj_set_pos(twelve, center_x - 10, -2);
  lv_obj_set_style_local_text_color(twelve, LV_LABEL_PART_MAIN, LV_STATE_DEFAULT, LV_COLOR_BLACK);

  six = lv_label_create(lv_scr_act(), nullptr);
  lv_label_set_align(six, LV_LABEL_ALIGN_CENTER);
  lv_label_set_text_static(six, "6");
  lv_obj_set_pos(six, center_x - 6, disp_height - 22);
  lv_obj_set_style_local_text_color(six, LV_LABEL_PART_MAIN, LV_STATE_DEFAULT, LV_COLOR_BLACK);

  batteryIcon.Create(lv_scr_act());
  lv_obj_align(batteryIcon.GetObject(), nullptr, LV_ALIGN_IN_TOP_RIGHT, -6, 6);

  plugIcon = lv_label_create(lv_scr_act(), nullptr);
  lv_label_set_text_static(plugIcon, Symbols::plug);
  lv_obj_set_style_local_text_color(plugIcon, LV_LABEL_PART_MAIN, LV_STATE_DEFAULT, LV_COLOR_BLACK);
  lv_obj_set_style_local_text_opa(plugIcon, LV_LABEL_PART_MAIN, LV_STATE_DEFAULT, 80);
  lv_obj_align(plugIcon, nullptr, LV_ALIGN_IN_TOP_RIGHT, -6, 6);

  bleIcon = lv_label_create(lv_scr_act(), nullptr);
  lv_label_set_text_static(bleIcon, Symbols::bluetooth);
  lv_obj_set_style_local_text_opa(bleIcon, LV_LABEL_PART_MAIN, LV_STATE_DEFAULT, 80);
  lv_obj_set_style_local_text_color(bleIcon, LV_LABEL_PART_MAIN, LV_STATE_DEFAULT, LV_COLOR_BLUE);
  lv_obj_align(bleIcon, nullptr, LV_ALIGN_IN_BOTTOM_RIGHT, -4, -4);

  heartbeatIcon = lv_label_create(lv_scr_act(), nullptr);
  lv_label_set_text_static(heartbeatIcon, Symbols::heartBeat);
  lv_obj_set_style_local_text_color(heartbeatIcon, LV_LABEL_PART_MAIN, LV_STATE_DEFAULT, lv_color_hex(0xCE1B1B));
  lv_obj_set_style_local_text_opa(heartbeatIcon, LV_LABEL_PART_MAIN, LV_STATE_DEFAULT, 80);
  lv_obj_align(heartbeatIcon, lv_scr_act(), LV_ALIGN_IN_BOTTOM_LEFT, 3, -3);

  heartbeatValue = lv_label_create(lv_scr_act(), nullptr);
  lv_obj_set_style_local_text_color(heartbeatValue, LV_LABEL_PART_MAIN, LV_STATE_DEFAULT, lv_color_hex(0xCE1B1B));
  lv_label_set_text_static(heartbeatValue, "");
  lv_obj_align(heartbeatValue, heartbeatIcon, LV_ALIGN_OUT_RIGHT_MID, 5, 0);

  notificationIcon = lv_label_create(lv_scr_act(), nullptr);
  lv_obj_set_style_local_text_color(notificationIcon, LV_LABEL_PART_MAIN, LV_STATE_DEFAULT, LV_COLOR_BLACK);
  lv_label_set_text_static(notificationIcon, NotificationIcon::GetIcon(false));
  lv_obj_align(notificationIcon, nullptr, LV_ALIGN_IN_TOP_LEFT, 0, 0);

  // Date - Day / Week day

  label_date_day = lv_label_create(lv_scr_act(), nullptr);
  lv_obj_set_style_local_text_color(label_date_day, LV_LABEL_PART_MAIN, LV_STATE_DEFAULT, lv_color_hex(0x333333));
  lv_label_set_align(label_date_day, LV_LABEL_ALIGN_RIGHT);
  lv_obj_align(label_date_day, nullptr, LV_ALIGN_IN_RIGHT_MID, -27, 0);

  minute_body = lv_line_create(lv_scr_act(), nullptr);
  hour_body = lv_line_create(lv_scr_act(), nullptr);
  second_body = lv_line_create(lv_scr_act(), nullptr);

  lv_style_init(&second_line_style);
  lv_style_set_line_width(&second_line_style, LV_STATE_DEFAULT, 2);
  lv_style_set_line_color(&second_line_style, LV_STATE_DEFAULT, LV_COLOR_RED);
  lv_style_set_line_rounded(&second_line_style, LV_STATE_DEFAULT, false);
  lv_obj_add_style(second_body, LV_LINE_PART_MAIN, &second_line_style);

  lv_style_init(&minute_line_style);
  lv_style_set_line_width(&minute_line_style, LV_STATE_DEFAULT, 1);
  lv_style_set_line_color(&minute_line_style, LV_STATE_DEFAULT, LV_COLOR_BLACK);
  lv_style_set_line_rounded(&minute_line_style, LV_STATE_DEFAULT, true);
  lv_style_set_line_opa(&minute_line_style, LV_STATE_DEFAULT, LV_OPA_100);
  lv_obj_add_style(minute_body, LV_LINE_PART_MAIN, &minute_line_style);

  lv_style_init(&hour_line_style);
  lv_style_set_line_width(&hour_line_style, LV_STATE_DEFAULT, 1);
  lv_style_set_line_color(&hour_line_style, LV_STATE_DEFAULT, LV_COLOR_BLACK);
  lv_style_set_line_rounded(&hour_line_style, LV_STATE_DEFAULT, true);
  lv_style_set_line_opa(&hour_line_style, LV_STATE_DEFAULT, LV_OPA_100);
  lv_obj_add_style(hour_body, LV_LINE_PART_MAIN, &hour_line_style);

  lv_style_init(&large_scale_style);
  lv_style_set_line_width(&large_scale_style, LV_STATE_DEFAULT, 1);
  lv_style_set_line_color(&large_scale_style, LV_STATE_DEFAULT, LV_COLOR_BLACK);
  lv_style_set_line_opa(&large_scale_style, LV_STATE_DEFAULT, LV_OPA_30);
  lv_style_set_line_rounded(&large_scale_style, LV_STATE_DEFAULT, false);

  lv_style_init(&medium_scale_style);
  lv_style_set_line_width(&medium_scale_style, LV_STATE_DEFAULT, 2);
  lv_style_set_line_color(&medium_scale_style, LV_STATE_DEFAULT, LV_COLOR_BLACK);
  lv_style_set_line_opa(&medium_scale_style, LV_STATE_DEFAULT, LV_OPA_70);
  lv_style_set_line_rounded(&medium_scale_style, LV_STATE_DEFAULT, false);

  lv_style_init(&small_scale_style);
  lv_style_set_line_width(&small_scale_style, LV_STATE_DEFAULT, 1);
  lv_style_set_line_color(&small_scale_style, LV_STATE_DEFAULT, LV_COLOR_BLACK);
  lv_style_set_line_opa(&small_scale_style, LV_STATE_DEFAULT, LV_OPA_30);
  lv_style_set_line_rounded(&small_scale_style, LV_STATE_DEFAULT, false);

  lv_style_init(&backdrop_style);
  lv_style_set_bg_opa(&backdrop_style, LV_STATE_DEFAULT, LV_OPA_COVER);
  lv_style_set_bg_color(&backdrop_style, LV_STATE_DEFAULT, backdrop_gradient[1]);
  lv_style_set_bg_grad_color(&backdrop_style, LV_STATE_DEFAULT, backdrop_gradient[0]);
  lv_style_set_bg_grad_dir(&backdrop_style, LV_STATE_DEFAULT, LV_GRAD_DIR_VER);
  lv_style_set_bg_grad_stop(&backdrop_style, LV_STATE_DEFAULT, disp_height - 1);
  lv_style_set_clip_corner(&backdrop_style, LV_STATE_DEFAULT, true);
  lv_style_set_radius(&backdrop_style, LV_STATE_DEFAULT, 12);
  lv_obj_add_style(backdrop, LV_OBJ_PART_MAIN, &backdrop_style);

  lv_style_init(&time_box_style);
  lv_style_set_bg_opa(&time_box_style, LV_STATE_DEFAULT, LV_OPA_60);
  lv_style_set_bg_color(&time_box_style, LV_STATE_DEFAULT, lv_color_hex(0x333333));
  lv_style_set_bg_grad_color(&time_box_style, LV_STATE_DEFAULT, lv_color_hex(0x000000));
  lv_style_set_bg_grad_dir(&time_box_style, LV_STATE_DEFAULT, LV_GRAD_DIR_VER);
  lv_style_set_bg_grad_stop(&time_box_style, LV_STATE_DEFAULT, disp_height - 1);
  lv_style_set_clip_corner(&time_box_style, LV_STATE_DEFAULT, true);
  lv_style_set_outline_width(&time_box_style, LV_STATE_DEFAULT, 40);
  lv_style_set_outline_color(&time_box_style, LV_STATE_DEFAULT, LV_COLOR_BLACK);
  lv_style_set_radius(&time_box_style, LV_STATE_DEFAULT, 8);
  lv_obj_add_style(time_box, LV_OBJ_PART_MAIN, &time_box_style);

  float maxRadius = fmin(disp_width, disp_height) / 2;
  CalculateSquircleRadii(scale_line_objs, maxRadius * 0.98, 2.4, 1, 1);

  taskRefresh = lv_task_create(RefreshTaskCallback, LV_DISP_DEF_REFR_PERIOD, LV_TASK_PRIO_MID, this);

  Refresh();
}

WatchFaceSquircle::~WatchFaceSquircle() {
  lv_task_del(taskRefresh);

  lv_style_reset(&hour_line_style);
  lv_style_reset(&minute_line_style);
  lv_style_reset(&second_line_style);
  lv_style_reset(&large_scale_style);
  lv_style_reset(&medium_scale_style);
  lv_style_reset(&small_scale_style);
  lv_style_reset(&backdrop_style);
  lv_style_reset(&time_box_style);

  lv_obj_clean(lv_scr_act());
}

/**
 * Rounds x,y coordinates that are floats to the nearest integers and places them where pointed as an lv_point_t
 */
void WatchFaceSquircle::NearestPoint(float x, float y, lv_point_t* point) {
  *point = {static_cast<lv_coord_t>(x), static_cast<lv_coord_t>(y)};
}

void toCart(float theta, float radius, lv_coord_t offset_x, lv_coord_t offset_y, lv_point_t* result) {
  *result = {static_cast<lv_coord_t>(radius * cosf(theta) + static_cast<float>(offset_x)),
             static_cast<lv_coord_t>(radius * sinf(theta) + static_cast<float>(offset_y))};
}

template <size_t N>
void WatchFaceSquircle::CalculateSquircleRadii(lv_obj_t* (&line_objs)[N], float size, float n, float a, float b) {
  float inverse_n = -1.0 / n;
  float scale = 1.0;
  for (size_t i = 0; i < N; i++) {
    float theta = (static_cast<float>(i) / static_cast<float>(N)) * TAU;
    float cos_t = cosf(theta);
    float sin_t = sinf(theta);
    // The superellipse formula gives the radius for use in a polar coordinate, name it (r, theta)
    // Theta is already known, use the formula to get the radius:
    float r1 = powf(powf(fabs(cos_t / a), n) + powf(fabs(sin_t / a), n), inverse_n) * size;
    toCart(theta, r1, center_x, center_y, &scales[i].points[0]);

    switch (i) {
      // 6
      case 15:
        continue;
      // 3 and 9
      case 59:
      case 0:
      case 1:
      case 29:
      case 30:
      case 31:
        continue;
      // case 0:
      // case 30:
      //   scale_line_objs[i] = lv_line_create(lv_scr_act(), nullptr);
      //   lv_obj_add_style(scale_line_objs[i], LV_LINE_PART_MAIN, &large_scale_style);
      //   scale = 0.95;
      //   break;
      //  12
      case 44:
      case 45:
      case 46:
        continue;
      case 5:
      case 10:
      case 20:
      case 25:
      case 35:
      case 40:
      case 50:
      case 55:
        scale_line_objs[i] = lv_line_create(lv_scr_act(), nullptr);
        lv_obj_add_style(scale_line_objs[i], LV_LINE_PART_MAIN, &medium_scale_style);
        scale = 0.92;
        break;
      default:
        scale_line_objs[i] = lv_line_create(lv_scr_act(), nullptr);
        lv_obj_add_style(scale_line_objs[i], LV_LINE_PART_MAIN, &small_scale_style);
        scale = 0.95;
        break;
    }
    float r2 = r1 * scale;
    toCart(theta, r2, center_x, center_y, &scales[i].points[1]);
    lv_line_set_points(line_objs[i], scales[i].points, 2);
  }
}

void WatchFaceSquircle::UpdateClock() {
  uint8_t latest_hour = dateTimeController.Hours();
  uint8_t latest_minute = dateTimeController.Minutes();
  uint8_t latest_second = dateTimeController.Seconds();
  float r1, r2, t, cos_t, sin_t;

  bool minute_needs_update = latest_minute != minute;
  if (minute_needs_update) {
    minute = latest_minute;

    r1 = 100; // minute hand length
    t = ((static_cast<float>(minute) / 60) * TAU) - PI_2;
    toCart(t + PI_2, 10, 0, 0, &minute_point[0]);
    toCart(t - PI_2, 10, 0, 0, &minute_point[1]);

    lv_line_set_points(minute_body, minute_point, 2);
  }

  if (latest_hour != hour || minute_needs_update) {
    hour = latest_hour;
    minute = latest_minute;

    r1 = 70; // hour hand length
    t = (static_cast<float>(hour % 12) * HOUR_SLICE - PI_2) + (static_cast<float>(minute) / 60 * HOUR_SLICE);
    cos_t = cosf(t);
    sin_t = sinf(t);

    NearestPoint(r1 * cos_t + center_x, r1 * sin_t + center_y, &hour_point[0]);
    hour_point[1] = {center_x, center_y};

    lv_line_set_points(hour_body, hour_point, 2);

    lv_label_set_text_fmt(label_time, "%02i:%02i", latest_hour % 12, latest_minute);
  }

  if (latest_second != second) {
    second = latest_second;

    r1 = 108; // second hand length
    r2 = -30; // back part of second hand length
    t = ((static_cast<float>(second) / 60) * TAU) - PI_2;
    cos_t = cosf(t);
    sin_t = sinf(t);

    NearestPoint(r1 * cos_t + center_x, r1 * sin_t + center_y, &second_point[0]);
    NearestPoint(r2 * cos_t + center_x, r2 * sin_t + center_y, &second_point[1]);

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

    currentDate = std::chrono::time_point_cast<std::chrono::days>(currentDateTime.Get());
    if (currentDate.IsUpdated()) {
      lv_label_set_text_fmt(label_date_day, "%s %02i", dateTimeController.DayOfWeekShortToString(), dateTimeController.Day());
    }
  }

  heartbeat = heartRateController.HeartRate();
  heartbeatRunning = heartRateController.State() != Controllers::HeartRateController::States::Stopped;
  if (heartbeat.IsUpdated() || heartbeatRunning.IsUpdated()) {
    if (heartbeatRunning.Get()) {
      lv_obj_set_style_local_text_color(heartbeatIcon, LV_LABEL_PART_MAIN, LV_STATE_DEFAULT, lv_color_hex(0xCE1B1B));
      lv_obj_set_style_local_text_opa(heartbeatIcon, LV_LABEL_PART_MAIN, LV_STATE_DEFAULT, 200);
      lv_label_set_text_fmt(heartbeatValue, "%d", heartbeat.Get());
      lv_obj_set_style_local_text_opa(heartbeatValue, LV_LABEL_PART_MAIN, LV_STATE_DEFAULT, 200);
    } else {
      lv_obj_set_style_local_text_color(heartbeatIcon, LV_LABEL_PART_MAIN, LV_STATE_DEFAULT, lv_color_hex(0x1B1B1B));
      lv_label_set_text_static(heartbeatValue, "");
    }

    lv_obj_realign(heartbeatIcon);
    lv_obj_realign(heartbeatValue);
  }
}
