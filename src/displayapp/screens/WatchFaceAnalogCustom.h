#pragma once

#include <chrono>
#include <cstdint>
#include <lvgl/src/lv_core/lv_obj.h>
#include <memory>
#include "components/battery/BatteryController.h"
#include "components/ble/BleController.h"
#include "components/ble/NotificationManager.h"
#include "components/datetime/DateTimeController.h"
#include "displayapp/screens/BatteryIcon.h"
#include "displayapp/screens/Screen.h"
#include "utility/DirtyValue.h"

namespace Pinetime {
  namespace Controllers {
    class Settings;
    class Battery;
    class Ble;
    class NotificationManager;
    class HeartRateController;
    class MotionController;
  }

  namespace Applications {
    namespace Screens {

      class WatchFaceAnalogCustom : public Screen {
      public:
        WatchFaceAnalogCustom(Controllers::DateTime& dateTimeController,
                        const Controllers::Battery& batteryController,
                        const Controllers::Ble& bleController,
                        Controllers::NotificationManager& notificationManager,
                        Controllers::Settings& settingsController,
                        Controllers::HeartRateController& heartRateController,
                        Controllers::MotionController& motionController);

        ~WatchFaceAnalogCustom() override;

        void Refresh() override;

      private:
        uint8_t sHour, sMinute, sSecond;

        Utility::DirtyValue<uint8_t> batteryPercentRemaining {0};
        Utility::DirtyValue<bool> isCharging {};
        Utility::DirtyValue<bool> bleState {};
        Utility::DirtyValue<std::chrono::time_point<std::chrono::system_clock, std::chrono::nanoseconds>> currentDateTime;
        Utility::DirtyValue<bool> notificationState {false};
        Utility::DirtyValue<std::chrono::time_point<std::chrono::system_clock, std::chrono::days>> currentDate;
        Utility::DirtyValue<uint32_t> stepCount {};
        Utility::DirtyValue<uint8_t> heartbeat {};
        Utility::DirtyValue<bool> heartbeatRunning {};

        lv_obj_t* minor_scales;
        lv_obj_t* major_scales;
        lv_obj_t* large_scales;
        lv_obj_t* twelve;

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

        lv_obj_t* label_time;
        lv_obj_t* label_year;
        lv_obj_t* label_date_day;
        lv_obj_t* label_week;
        lv_obj_t* plugIcon;
        lv_obj_t* notificationIcon;
        lv_obj_t* bleIcon;
        lv_obj_t* heartbeatIcon;
        lv_obj_t* heartbeatValue;
        lv_obj_t* stepIcon;
        lv_obj_t* stepValue;

        BatteryIcon batteryIcon;

        Controllers::DateTime& dateTimeController;
        const Controllers::Battery& batteryController;
        const Controllers::Ble& bleController;
        Controllers::NotificationManager& notificationManager;
        Controllers::Settings& settingsController;
        Controllers::HeartRateController& heartRateController;
        Controllers::MotionController& motionController;

        void UpdateClock();
        void SetBatteryIcon();

        lv_task_t* taskRefresh;
      };
    }

    template <>
    struct WatchFaceTraits<WatchFace::AnalogCustom> {
      static constexpr WatchFace watchFace = WatchFace::AnalogCustom;
      static constexpr const char* name = "A-Custom";

      static Screens::Screen* Create(AppControllers& controllers) {
        return new Screens::WatchFaceAnalogCustom(controllers.dateTimeController,
                                            controllers.batteryController,
                                            controllers.bleController,
                                            controllers.notificationManager,
                                            controllers.settingsController,
                                            controllers.heartRateController,
                                            controllers.motionController);
      };

      static bool IsAvailable(Pinetime::Controllers::FS& /*filesystem*/) {
        return true;
      }
    };
  }
}
