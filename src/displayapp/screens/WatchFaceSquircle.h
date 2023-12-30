#pragma once

#include <lvgl/src/lv_core/lv_obj.h>
#include <chrono>
#include <cstdint>
#include <memory>
#include "displayapp/screens/Screen.h"
#include "components/datetime/DateTimeController.h"
#include "components/battery/BatteryController.h"
#include "components/ble/BleController.h"
#include "components/heartrate/HeartRateController.h"
#include "components/ble/NotificationManager.h"
#include "displayapp/screens/BatteryIcon.h"
#include "utility/DirtyValue.h"
#include "displayapp/InfiniTimeTheme.h"

namespace Pinetime {
  namespace Controllers {
    class Settings;
    class Battery;
    class Ble;
    class HeartRateController;
    class NotificationManager;
  }

  namespace Applications {
    namespace Screens {

      typedef struct {
        lv_point_t points[2];
      } line_segment;

      class WatchFaceSquircle : public Screen {
      public:
        WatchFaceSquircle(Controllers::DateTime& dateTimeController,
                          const Controllers::Battery& batteryController,
                          const Controllers::Ble& bleController,
                          Controllers::NotificationManager& notificationManager,
                          Controllers::HeartRateController& heartRateController,
                          Controllers::Settings& settingsController);

        ~WatchFaceSquircle() override;

        void Refresh() override;

      private:
        uint8_t second, minute, hour, twelveHour;
        float secondf, minutef, hourf, twelveHourf;

        Utility::DirtyValue<uint8_t> batteryPercentRemaining {0};
        Utility::DirtyValue<bool> isCharging {};
        Utility::DirtyValue<bool> bleState {};
        Utility::DirtyValue<std::chrono::time_point<std::chrono::system_clock, std::chrono::nanoseconds>> currentDateTime;
        Utility::DirtyValue<bool> notificationState {false};
        using days = std::chrono::duration<int32_t, std::ratio<86400>>; // TODO: days is standard in c++20
        Utility::DirtyValue<std::chrono::time_point<std::chrono::system_clock, days>> currentDate;

        Utility::DirtyValue<uint8_t> heartbeat {};
        Utility::DirtyValue<bool> heartbeatRunning {};

        lv_obj_t* twelve;

        lv_obj_t* hour_body;
        lv_obj_t* minute_body;
        lv_obj_t* second_body;
        lv_obj_t* backdrop;
        lv_obj_t* hour_scale_line_objs[12];

        lv_point_t hour_point[2];
        lv_point_t minute_point[2];
        lv_point_t second_point[2];

        lv_style_t hour_line_style;
        lv_style_t minute_line_style;
        lv_style_t second_line_style;
        lv_style_t hour_scale_style;
        lv_style_t backdrop_style;

        lv_obj_t* label_date_day;
        lv_obj_t* plugIcon;
        lv_obj_t* notificationIcon;
        lv_obj_t* bleIcon;
        lv_obj_t* heartbeatIcon;
        lv_obj_t* heartbeatValue;

        line_segment scales[12];

        const lv_color_t color_bg_grad_top = lv_color_hex(0xFDF8DC);
        const lv_color_t color_bg_grad_bottom = lv_color_hex(0xA3AEB0);
        const lv_color_t color_hour_scales = LV_COLOR_BLACK;
        const lv_color_t color_hour_minute_hands = LV_COLOR_BLACK;
        const lv_color_t color_second_hand = LV_COLOR_RED;
        const lv_color_t color_date = LV_COLOR_BLACK;
        const lv_color_t color_time = LV_COLOR_BLACK;
        const lv_color_t color_battery_high = LV_COLOR_BLACK;
        const lv_color_t color_battery_low = Colors::deepOrange;
        const lv_color_t color_battery_critical = LV_COLOR_RED;
        const lv_style_int_t width_hour_hand = 5;
        const lv_style_int_t width_minute_hand = 5;
        const lv_style_int_t width_second_hand = 2;
        const lv_style_int_t width_hour_scales = 4;
        const float length_minute_hand = 100;
        const float length_hour_hand = 70;
        const float length_second_hand = 108;
        const float length_second_hand_back = -30;

        BatteryIcon batteryIcon;

        const Controllers::DateTime& dateTimeController;
        const Controllers::Battery& batteryController;
        const Controllers::Ble& bleController;
        Controllers::NotificationManager& notificationManager;
        Controllers::Settings& settingsController;
        Controllers::HeartRateController& heartRateController;

        void UpdateClock();
        void SetBatteryIcon();

        void NearestPoint(float x, float y, lv_point_t* point);

        template <size_t N>
        void CalculateSquircleRadii(lv_obj_t* (&line_objs)[N], float size, float n, float a, float b);

        lv_task_t* taskRefresh;
      };
    }

    template <>
    struct WatchFaceTraits<WatchFace::Squircle> {

      static constexpr WatchFace watchFace = WatchFace::Squircle;
      static constexpr const char* name = "Squircle face";

      static Screens::Screen* Create(AppControllers& controllers) {
        return new Screens::WatchFaceSquircle(controllers.dateTimeController,
                                              controllers.batteryController,
                                              controllers.bleController,
                                              controllers.notificationManager,
                                              controllers.heartRateController,
                                              controllers.settingsController);
      };

      static bool IsAvailable(Pinetime::Controllers::FS& /*filesystem*/) {
        return true;
      }
    };
  }
}