#pragma once

#include "esphome/components/climate_ir/climate_ir.h"

namespace esphome {
namespace climate_ir_YKRL {

// Temperature
const uint8_t TEMP_MIN = 16;  // Celsius
const uint8_t TEMP_MAX = 32;  // Celsius

const uint16_t CARRIER_FRECUENCY = 38000;

const uint16_t LEAD_IN_MARK = 9020;
const uint16_t LEAD_IN_SPACE = 4520;
const uint16_t LEAD_OUT_SPACE = 50000; // Message space
const uint16_t BIT_MARK = 552;
const uint16_t ZERO_SPACE = 657;
const uint16_t ONE_SPACE = 1735;

class YKRLClimate : public climate_ir::ClimateIR {
    public:
        YKRLClimate()
            : climate_ir::ClimateIR(TEMP_MIN, TEMP_MAX, 0.5f, true, true,
                                    {climate::CLIMATE_FAN_AUTO, climate::CLIMATE_FAN_LOW, climate::CLIMATE_FAN_MEDIUM, climate::CLIMATE_FAN_HIGH},
                                    {climate::CLIMATE_SWING_OFF, climate::CLIMATE_SWING_VERTICAL, climate::CLIMATE_SWING_HORIZONTAL},
                                    {climate::CLIMATE_PRESET_ECO, climate::CLIMATE_PRESET_BOOST, climate::CLIMATE_PRESET_SLEEP, climate::CLIMATE_PRESET_COMFORT}) {}
    protected:
        /// Transmit via IR the state of this climate controller.
        void transmit_state() override;
        /// Handle received IR Buffer
        bool on_receive(remote_base::RemoteReceiveData data) override;

        // void calc_checksum_(uint32_t &value);

        climate::ClimateMode mode_before_{climate::CLIMATE_MODE_OFF};
};

}  // namespace climate_ir_YKRL
}  // namespace esphome