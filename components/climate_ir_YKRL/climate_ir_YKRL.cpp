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
    modes.insert(climate::CLIMATE_MODE_AUTO);
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
    uint8_t state_frame[STATE_FRAME_SIZE] = {};

    if (!data.expect_item(LEAD_IN_MARK,LEAD_IN_SPACE)){
        return false;
    }
    ESP_LOGCONFIG(TAG, "LEAD_IN");


    for (uint8_t pos = 0; pos < STATE_FRAME_SIZE; pos++) {
        uint8_t byte = 0;
        for (int8_t bit = 0; bit < 8; bit++) {
            if (data.expect_item(BIT_MARK, ONE_SPACE)) {
                byte |= 1 << bit;
            } else if (!data.expect_item(BIT_MARK, ZERO_SPACE)) {
                return false;
            }
        }
        ESP_LOGCONFIG(TAG, "RX[%i] %x", pos, byte);
        state_frame[pos] = byte;
    }
    // SKIP AT THIS MOMENT
    // if (!data.expect_item(BIT_MARK,LEAD_OUT_SPACE)){
    //     return false;
    // }

    ESP_LOGCONFIG(TAG, "MESSAGE");

    return parse_state_frame_(state_frame);
}

bool YKRLClimate::parse_state_frame_(const uint8_t frame[]){
    uint16_t checksum = 0;

    for (int i = 0; i < STATE_FRAME_SIZE - 1; i++){
        checksum += frame[i];
    }
    checksum = checksum & 0xFF;

    if (frame[STATE_FRAME_SIZE-1] != checksum){
        ESP_LOGCONFIG(TAG, "Bad checksum %x", checksum);
        return false;
    }

    uint8_t temp = frame[1] >> 3;
    if (temp > 0){
        this->target_temperature = frame[3] >> 7 ? temp + TEMP_OFFSET + 0.5f : temp + TEMP_OFFSET;
    }
    ESP_LOGCONFIG(TAG, "Temperature %f", this->target_temperature);

    uint8_t modeset = frame[6] & 0xE0;
    switch(modeset){
        case 0x20:
            this->mode = climate::CLIMATE_MODE_COOL;
            break;
        case 0xC0:
            this->mode = climate::CLIMATE_MODE_FAN_ONLY;
            break;
        case 0x40:
            this->mode = climate::CLIMATE_MODE_DRY;
            break;
        case 0x80:
            this->mode = climate::CLIMATE_MODE_HEAT;
            break;
        case 0x00:
        default:
            this->mode = climate::CLIMATE_MODE_AUTO;
    }

    uint8_t on_off = frame[9] & 0x30;
    if (on_off == 0x00){
        this->mode = climate::CLIMATE_MODE_OFF;
    }

    ESP_LOGCONFIG(TAG, "Mode %x", modeset);
    ESP_LOGCONFIG(TAG, "Mode %x", this->mode);


    this->publish_state();

    return true;

}

uint8_t YKRLClimate::temperature_(){
    switch(this->mode){
        case climate::CLIMATE_MODE_FAN_ONLY:
        case climate::CLIMATE_MODE_AUTO:
            return 0x00;
        default:
            return ((uint8_t) int(this->target_temperature) - TEMP_OFFSET)<<3;
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