#include "components/2A03.h"

RP2A03::RP2A03() {

    m_connectors["OAMDMA"] = std::make_shared<Connector>(DataInterface{
            .read = [&](uint32_t address, uint32_t & buffer) {
                return false;
            },
            .write = [&](uint32_t address, uint32_t data) {

                // Dump contents of 0xXX00-0xXXFF to OAM memory through OAMDATA register.
                // High byte of address is determined by write to this register (0x4014).
                if(address == 0x4014) {
                    for(int index = 0; index <= 0xFF; index++) {
                        m_mainBus.write(0x2004, m_mainBus.read(((data & 0xFF) << 8) | index));
                    }
                    m_cycles += 513;
                }
            }
    });
}

RP2A03::~RP2A03() { }

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
