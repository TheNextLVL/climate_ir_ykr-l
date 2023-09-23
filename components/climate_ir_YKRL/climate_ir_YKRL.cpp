#include "climate_ir_YKRL.h"
#include "esphome/core/log.h"

namespace esphome {
namespace climate_ir_YKRL {

static const char *const TAG = "climate.climate_ir_YKRL";

void YKRLClimate::transmit_state() {
    // At this moment do nothing
    // Just sent AUTO ON or AUTO OFF
    uint8_t remote_state[13] = { 0xc3, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };
    if (this->mode == climate::CLIMATE_MODE_OFF){
        remote_state[1] = 0xe0; 
        remote_state[2] = 0x07; 
        remote_state[4] = 0x05;
        remote_state[11] = 0xa0;
        remote_state[12] = 0xf2; // checksum
    } else {
        //uint8_t remote_state[12] = { 0xc3, 0xe0, 0x07, 0x00, 0x05, 0x00, 0x00, 0x00, 0x00, 0x04, 0x00, 0xa0 };
        remote_state[1] = 0xe0; 
        remote_state[2] = 0x07; 
        remote_state[4] = 0x05;
        remote_state[9] = 0x04;
        remote_state[11] = 0xa0;
        remote_state[12] = 0xf6; // checksum
    }

    //Prepare to transmit
    auto transmit = this->transmitter_->transmit();
    auto *data = transmit.get_data();
    data->set_carrier_frequency(CARRIER_FRECUENCY);
    
    //Send LeadIn
    data->mark(LEAD_IN_MARK);
    data->space(LEAD_IN_SPACE);

    //Set data
    // Iterate trough each frame
    bool bit;
    for (int i = 0; i < 13; i++){
        for (uint8_t mask = 128; mask >0; mask >>=1){ //swap bit trough mask
            data -> mark(BIT_MARK);
            bit = remote_state[i] & mask;
            data->space(bit ? ONE_SPACE : ZERO_SPACE);
        }
    }
    data -> mark(BIT_MARK);
    data -> space(LEAD_OUT_SPACE);

    transmit.perform();
}

bool YKRLClimate::on_receive(remote_base::RemoteReceiveData data){
    //TODO
    return true;
}
}  // namespace climate_ir_YKRL
}  // namespace esphome