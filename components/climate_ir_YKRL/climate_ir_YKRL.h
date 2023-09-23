#pragma once

#include "esphome/components/climate_ir/climate_ir.h"

namespace esphome {
namespace climate_ir_YKRL {

// Temperature
const uint8_t TEMP_MIN = 16;  // Celsius
const uint8_t TEMP_MAX = 32;  // Celsius
const uint8_t STATE_FRAME_SIZE = 13;  // Number of bytes to send
const uint8_t TEMP_OFFSET = 8;  // Temperature offset to get same bits as in ir code

const uint16_t CARRIER_FRECUENCY = 38000;
const uint16_t LEAD_IN_MARK = 9020;
const uint16_t LEAD_IN_SPACE = 4520;
const uint16_t LEAD_OUT_SPACE = 50000; // Message space
const uint16_t BIT_MARK = 552;
const uint16_t ZERO_SPACE = 657;
const uint16_t ONE_SPACE = 1735;

const uint8_t VSWING_OFF = 0x07;
const uint8_t VSWING_AUTO = 0x03;
const uint8_t HSWING_OFF = 0x00;
const uint8_t HSWING_AUTO = 0xE0;


class YKRLClimate : public climate_ir::ClimateIR {
    public:
        YKRLClimate()
            : climate_ir::ClimateIR(TEMP_MIN, TEMP_MAX, 0.5f, true, true,
                                    {climate::CLIMATE_FAN_AUTO, climate::CLIMATE_FAN_QUIET, climate::CLIMATE_FAN_LOW, climate::CLIMATE_FAN_MEDIUM, climate::CLIMATE_FAN_HIGH},
                                    {climate::CLIMATE_SWING_OFF, climate::CLIMATE_SWING_VERTICAL, climate::CLIMATE_SWING_HORIZONTAL, climate::CLIMATE_SWING_BOTH},
                                    {climate::CLIMATE_PRESET_ECO, climate::CLIMATE_PRESET_BOOST, climate::CLIMATE_PRESET_SLEEP, climate::CLIMATE_PRESET_COMFORT}) {}
    protected:
        /// Set the supported modes
        climate::ClimateTraits traits() override;
        /// Transmit via IR the state of this climate controller.
        void transmit_state() override;
        /// Handle received IR Buffer
        bool on_receive(remote_base::RemoteReceiveData data) override;
        /// Parse the ir received frame
        bool parse_state_frame_(const uint8_t frame[]);

        /// Calculate values depending of the mode,temp,etc...
        uint8_t temperature_();
        uint8_t fan_speed_();
        uint8_t operation_mode_();
        uint8_t vswing_();
        uint8_t hswing_();
        uint8_t ext_fan_();
        uint8_t mode_();

        climate::ClimateMode mode_before_{climate::CLIMATE_MODE_OFF};
};

}  // namespace climate_ir_YKRL
}  // namespace esphome