#include "climate_ir_YKRL.h"
#include "esphome/core/log.h"

namespace esphome {
namespace climate_ir_YKRL {

static const char *const TAG = "climate.climate_ir_YKRL";

// Modify supported modes to quit HEAT_COOL
climate::ClimateTraits YKRLClimate::traits(){
    auto traits = ClimateIR::traits();
    auto modes = traits.get_supported_modes();
    modes.erase(climate::CLIMATE_MODE_HEAT_COOL);
    traits.set_supported_modes(modes);
    return traits;
}

// At this moment do nothing
// Just sent AUTO ON or AUTO OFF
void YKRLClimate::transmit_state() {
    uint16_t checksum = 0;
    uint8_t remote_state[STATE_FRAME_SIZE] = {0};

    // Byte0 - Set produc id
    remote_state[0] = 0xC3;
    // Byte1 - temperature + hswing
    remote_state[1] = temperature_() + vswing_();
    // Byte2 - vswing
    remote_state[2] = hswing_();
    // Byte3 - Temperature step .5
    remote_state[3] = this->target_temperature == int(this->target_temperature) ? 0x00 : 0x80;
    // Byte4 - Fan speed
    remote_state[4] = fan_speed_();
    // Byte5  - Extended fan modes
    remote_state[5] = ext_fan_();
    // Byte6 - mode
    remote_state[6] = mode_();
    // Byte 9 - ON/OFF
    remote_state[9] = this->mode == climate::CLIMATE_MODE_OFF ? 0x00 : 0x20;
    // Byte 11 - ¿Random=
    remote_state[11] = 0x05;

    //Calculate checksum
    for (uint8_t i : remote_state){
        checksum += i;
    }
    remote_state[12] = checksum & 0xFF;


    //Prepare to transmit
    auto transmit = this->transmitter_->transmit();
    auto *data = transmit.get_data();
    data->set_carrier_frequency(CARRIER_FRECUENCY);
    
    //Send LeadIn
    data->mark(LEAD_IN_MARK);
    data->space(LEAD_IN_SPACE);

    //Set data
    bool bit;
    for (int i = 0; i < 13; i++){
        for (uint8_t mask = 1; mask >0; mask <<=1){ //swap bit trough mask
            data -> mark(BIT_MARK);
            bit = remote_state[i] & mask;
            data->space(bit ? ONE_SPACE : ZERO_SPACE);
        }
    }

    //Send LeadOut
    data -> mark(BIT_MARK);
    data -> space(LEAD_OUT_SPACE);

    transmit.perform();
}

bool YKRLClimate::on_receive(remote_base::RemoteReceiveData data){
    //TODO
    return true;
}

uint8_t YKRLClimate::temperature_(){
    switch(this->mode){
        case climate::CLIMATE_MODE_FAN_ONLY:
        case climate::CLIMATE_MODE_AUTO:
            return 0x00;
        default:
            return ((uint8_t) roundf(this->target_temperature) - TEMP_OFFSET)<<3;
    }
}

uint8_t YKRLClimate::vswing_(){
    switch(this->swing_mode){
        case climate::CLIMATE_SWING_HORIZONTAL:
        case climate::CLIMATE_SWING_BOTH:
            return VSWING_AUTO;
        case climate::CLIMATE_SWING_OFF:
        case climate::CLIMATE_SWING_VERTICAL:
        default:
            return VSWING_OFF;
    }
}

uint8_t YKRLClimate::hswing_(){
    switch(this->swing_mode){
        case climate::CLIMATE_SWING_VERTICAL:
        case climate::CLIMATE_SWING_BOTH:
            return HSWING_AUTO;
        case climate::CLIMATE_SWING_OFF:
        case climate::CLIMATE_SWING_HORIZONTAL:
        default:
            return HSWING_OFF;
    }
}

uint8_t YKRLClimate::fan_speed_(){
    switch(this->fan_mode.value()){
        case climate::CLIMATE_FAN_FOCUS:
        case climate::CLIMATE_FAN_HIGH:
            return 0x20;
        case climate::CLIMATE_FAN_MEDIUM:
            return 0x40;
        case climate::CLIMATE_FAN_LOW:
            return 0x60;
        case climate::CLIMATE_FAN_QUIET:
        case climate::CLIMATE_FAN_AUTO:
        default:
            return 0xA0;
    }
}

uint8_t YKRLClimate::ext_fan_(){
    switch(this->fan_mode.value()){
        case climate::CLIMATE_FAN_FOCUS:
            return 0x40;
        case climate::CLIMATE_FAN_QUIET:
            return 0x80;
        default:
            return 0x00;
    }
}

uint8_t YKRLClimate::mode_(){
    switch(this->mode){
        case climate::CLIMATE_MODE_COOL:
            return 0x20;
        case climate::CLIMATE_MODE_DRY:
            return 0x40;
        case climate::CLIMATE_MODE_HEAT:
            return 0x80;
        case climate::CLIMATE_MODE_FAN_ONLY:
            return 0xC0;
        case climate::CLIMATE_MODE_AUTO:
        default:
            return 0x00;
    }
}

}  // namespace climate_ir_YKRL
}  // namespace esphome