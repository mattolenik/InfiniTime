#pragma once

#include <lvgl/src/lv_core/lv_obj.h>
#include <chrono>
#include <cstdint>
#include <memory>
#include "displayapp/screens/Screen.h"
#include "components/datetime/DateTimeController.h"
#include "components/battery/BatteryController.h"
#include "components/ble/BleController.h"
#include "components/ble/NotificationManager.h"
#include "displayapp/screens/BatteryIcon.h"
#include "utility/DirtyValue.h"

namespace Pinetime {
  namespace Controllers {
    class Settings;
    class Battery;
    class Ble;
    class NotificationManager;
  }

  namespace Applications {
    namespace Screens {

      class WatchFaceSquircle : public Screen {
      public:
        WatchFaceSquircle(Controllers::DateTime& dateTimeController,
                          const Controllers::Battery& batteryController,
                          const Controllers::Ble& bleController,
                          Controllers::NotificationManager& notificationManager,
                          Controllers::Settings& settingsController);

        ~WatchFaceSquircle() override;

        void Refresh() override;

      private:
        uint8_t sHour, sMinute, sSecond;

        Utility::DirtyValue<uint8_t> batteryPercentRemaining {0};
        Utility::DirtyValue<bool> isCharging {};
        Utility::DirtyValue<bool> bleState {};
        Utility::DirtyValue<std::chrono::time_point<std::chrono::system_clock, std::chrono::nanoseconds>> currentDateTime;
        Utility::DirtyValue<bool> notificationState {false};
        using days = std::chrono::duration<int32_t, std::ratio<86400>>; // TODO: days is standard in c++20
        Utility::DirtyValue<std::chrono::time_point<std::chrono::system_clock, days>> currentDate;

        lv_obj_t* twelve;

        lv_obj_t* hour_body;
        lv_obj_t* minute_body;
        lv_obj_t* second_body;

        lv_point_t hour_point[2];
        lv_point_t minute_point[2];
        lv_point_t second_point[2];

        lv_style_t hour_line_style;
        lv_style_t minute_line_style;
        lv_style_t second_line_style;

        lv_obj_t* label_date_day;
        lv_obj_t* plugIcon;
        lv_obj_t* notificationIcon;
        lv_obj_t* bleIcon;
        lv_point_t scale_hour_points_outer[12];
        lv_point_t scale_hour_points_inner[12];

        BatteryIcon batteryIcon;

        const Controllers::DateTime& dateTimeController;
        const Controllers::Battery& batteryController;
        const Controllers::Ble& bleController;
        Controllers::NotificationManager& notificationManager;
        Controllers::Settings& settingsController;

        void UpdateClock();
        void SetBatteryIcon();

        void NearestPoint(float x, float y, lv_point_t* point);

        template <size_t P>
        void DrawSquircle(lv_point_t (&points)[P]);

        template <size_t P>
        void CalculateSquirclePoints(lv_point_t (&points)[P],
                                     lv_coord_t pos_x,
                                     lv_coord_t pos_y,
                                     float radius,
                                     float n = 3,
                                     float a = 1,
                                     float b = 1);

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
                                              controllers.settingsController);
      };

      static bool IsAvailable(Pinetime::Controllers::FS& /*filesystem*/) {
        return true;
      }
    };
  }
}
