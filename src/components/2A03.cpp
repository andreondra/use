#include "components/2A03.h"

void RP2A03::init() {

    // Init CPU part.
    MOS6502::init();

    // Init 2A03-specific parts.
    for(uint16_t i = 0; i <= 0xF; i++){
        m_mainBus.write(0x4000 + i, 0x00);
    }

    for(uint16_t i : {0x4017, 0x4015, 0x4010, 0x4011, 0x4012, 0x4013}){
        m_mainBus.write(i, 0x00);
    }

}