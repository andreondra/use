#include "components/Gamepak/Mapper001.h"
#include "Types.h"
#include <stdexcept>

Mapper001::Mapper001(std::vector<uint8_t> & PRGROM, std::vector<uint8_t> & CHRROM, size_t PRGRAMSize)
        : m_PRGROM(PRGROM), m_CHRROM(CHRROM) {

    if(m_PRGROM.empty())
        throw std::invalid_argument("PRG ROM can't be empty.");

    // Check CHR ROM size
    if(m_CHRROM.empty()) {
        m_CHRRAM.resize(0x2000, 0x0);
        m_CHRROM = m_CHRRAM;
        m_CHRWritable = true;
    } else if(m_CHRROM.size() > 0x20000) {
        throw std::invalid_argument("CHR ROM invalid size: allowed max 128 KiB");
    }

    // Check PRG RAM size
    if(PRGRAMSize < 0x2000 || PRGRAMSize > 0x8000) {
        throw std::invalid_argument("PRG RAM invalid size: allowed 0x2000-0x8000.");
    }

    Mapper::init();
    m_PRGRAM.resize(PRGRAMSize, 0x00);
    m_registers.init();
}

void Mapper001::setMirroring(uint8_t rawValue) {
    switch(rawValue) {
        case 0: m_mirroringType = mirroringType_t::SINGLE_LO; break;
        case 1: m_mirroringType = mirroringType_t::SINGLE_HI; break;
        case 2: m_mirroringType = mirroringType_t::VERTICAL; break;
        case 3: m_mirroringType = mirroringType_t::HORIZONTAL; break;
        default: break;
    }
}

void Mapper001::init(){

    Mapper::init();
    m_registers.init();
    std::fill(m_PRGRAM.begin(), m_PRGRAM.end(), 0x00);

    m_loadRegister = 0;
    m_writeCounter = 0;
}

bool Mapper001::cpuRead(uint16_t addr, uint8_t & data) {

    // PRG RAM bank.
    if(addr >= 0x6000 && addr <= 0x7FFF){
        data = m_PRGRAM[(m_registers.PRGRAMSelect << 13) | (addr & 0x1FFF)];
        return true;

    // PRG ROM bank 0.
    } else if(addr >= 0x8000 && addr <= 0xBFFF) {

        addr &= 0x3FFF;

        switch(m_registers.PRGMode) {
            case PRGMode_t::SWITCH_BOTH0:
            [[fallthrough]];
            case PRGMode_t::SWITCH_BOTH1:
                data = m_PRGROM[addr | ((m_registers.PRGROMSelect & 0x1E) << 14)];
                break;
            case PRGMode_t::FIX_LOW_SWITCH_HIGH:
                data = m_PRGROM[addr];
                break;
            case PRGMode_t::SWITCH_LOW_FIX_HIGH:
                data = m_PRGROM[addr | (m_registers.PRGROMSelect << 14)];
                break;
        }

        return true;

    } else if(addr >= 0xC000 && addr <= 0xFFFF) {

        addr &= 0x7FFF;

        switch(m_registers.PRGMode) {
            case PRGMode_t::SWITCH_BOTH0:
                [[fallthrough]];
            case PRGMode_t::SWITCH_BOTH1:
                data = m_PRGROM[addr | ((m_registers.PRGROMSelect & 0x1E) << 14)];
                break;
            case PRGMode_t::FIX_LOW_SWITCH_HIGH:
                addr &= 0x3FFF;
                data = m_PRGROM[addr | (m_registers.PRGROMSelect << 14)];
                break;
            case PRGMode_t::SWITCH_LOW_FIX_HIGH:
                addr &= 0x3FFF;
                data = m_PRGROM[addr | (((m_PRGROM.size() / 0x4000) - 1) << 14)];
                break;
        }

        return true;
    }

    return false;
}

/**
 * CPU write interface.
 *
 * 0x6000-0x7FFF: PRG RAM
 * 0x8000-0xFFFF: configuration serial port
 * */
bool Mapper001::cpuWrite(uint16_t addr, uint8_t data){

    // Built-in PRG RAM area.
    if(addr >= 0x6000 && addr <= 0x7FFF){

        m_PRGRAM[(m_registers.PRGRAMSelect << 13) | (addr & 0x1FFF)] = data;
        return true;

    // Switchable PRG ROM bank.
    // Because it is a ROM, CPU writes are directed to the serial port instead.
    //
    // 7  bit  0
    // ---- ----
    // Rxxx xxxD
    // |       |
    // |       +- Data bit to be shifted into shift register, LSB first
    // +--------- A write with bit set will reset shift register
    //            and write Control with (Control OR $0C),
    //            locking PRG ROM at $C000-$FFFF to the last bank.
    // Diagram borrowed from: https://www.nesdev.org/wiki/MMC1.
    //
    // If the R bit is not set, then first 4 writes are loaded to the shift register,
    // 5th write will use all 5 data bits depending on the last address (only address used for 5th write matters).
    //
    // Only 13th and 14th address bit are used, thus making 4 usable locations.
    } else if(addr >= 0x8000 && addr <= 0xFFFF){

        // Reset bit active. Clean shifter and lock PRG ROM at 0xC000-0xFFFF.
        if(data & 0x80){

            m_writeCounter = 0;
            m_loadRegister = 0;
            m_registers.PRGMode = PRGMode_t::SWITCH_LOW_FIX_HIGH;
        } else {

            m_loadRegister = (m_loadRegister >> 1) | ((data & 0x1) << 4);
            m_loadRegister &= 0x1F;
            m_writeCounter++;

            if(m_writeCounter >= 5) {

                // Incomplete address decoding, only lines 13 and 14 are used.
                switch((addr & 0x6000) >> 13){

                    // Control register.
                    case 0:
                        setMirroring(m_loadRegister & 0x3);
                        m_registers.PRGMode = static_cast<PRGMode_t>((m_loadRegister & 0xC) >> 2);
                        m_registers.CHRMode = static_cast<CHRMode_t>((m_loadRegister & 0x10) >> 4);
                        break;

                    // CHR 0 control register.
                    case 1:
                        m_registers.CHRROMLoSelect = m_loadRegister & 0x1F;

                        // For 32 KB PRG RAM both bits are used.
                        if(m_PRGRAM.size() == 0x8000)
                            m_registers.PRGRAMSelect = (m_loadRegister & 0xC) >> 0x2;
                        // For 16 KB PRG RAM only high bit is used.
                        else
                            m_registers.PRGRAMSelect = (m_loadRegister & 0x8) >> 0x3;
                        break;

                        if(m_PRGROM.size() == 0x80000) {
                            m_registers.PRGROMSelect &= 0xF;
                            m_registers.PRGROMSelect |= m_loadRegister & 0x10;
                        }

                    // CHR 1 control register.
                    case 2:
                        m_registers.CHRROMHiSelect = m_loadRegister & 0x1F;

                        // For 32 KB PRG RAM both bits are used.
                        if(m_PRGRAM.size() == 0x4000)
                            m_registers.PRGRAMSelect = (m_loadRegister & 0xC) >> 0x2;
                        // For 16 KB PRG RAM only high bit is used.
                        else
                            m_registers.PRGRAMSelect = (m_loadRegister & 0x8) >> 0x3;
                        break;

                        if(m_PRGROM.size() == 0x80000) {
                            m_registers.PRGROMSelect &= 0xF;
                            m_registers.PRGROMSelect |= m_loadRegister & 0x10;
                        }

                    // PRG control register.
                    case 3:
                        m_registers.PRGROMSelect &= 0x10;
                        m_registers.PRGROMSelect |= m_loadRegister & 0xF;
                        m_registers.enablePRGRAM = !(m_loadRegister & 0x10);
                        break;
                }

                // Avoiding index overflow in a case of a bad program.
                // Calculate a count of units.
                // A14-A18 = Select 16 KB bank
                m_registers.PRGROMSelect %= m_PRGROM.size() / 0x4000;
                // A13-A14 = Select 8 KB bank
                m_registers.PRGRAMSelect %= m_PRGRAM.size() / 0x2000;
                // A12-A16 = Select 4 KB bank.
                m_registers.CHRROMLoSelect %= m_CHRROM.size() / 0x1000;
                m_registers.CHRROMHiSelect %= m_CHRROM.size() / 0x1000;

                m_writeCounter = 0;
                m_loadRegister = 0;
            }
        }


        return true;
    }

    return false;
}

bool Mapper001::ppuRead(uint16_t addr, uint8_t & data){

    if(addr >= 0x0000 && addr <= 0x0FFF){

        if(m_registers.CHRMode == CHRMode_t::SWITCH4KB){
            data = m_CHRROM[addr | (m_registers.CHRROMLoSelect << 12)];
        } else {
            data = m_CHRROM[addr | ((m_registers.CHRROMLoSelect & 0x1E) << 12)];
        }

        return true;

    } else if(addr >= 0x1000 && addr <= 0x1FFF){

        if(m_registers.CHRMode == CHRMode_t::SWITCH4KB){
            data = m_CHRROM[(addr & 0xFFF) | (m_registers.CHRROMHiSelect << 12)];
        } else {
            data = m_CHRROM[addr | ((m_registers.CHRROMHiSelect & 0x1E) << 12)];
        }

        return true;
    } else if (addr >= 0x2000 && addr <= 0x3EFF) {

        data = CIRAMRead(addr);
        return true;
    }

    return false;
}

bool Mapper001::ppuWrite(uint16_t addr, uint8_t data){

    if(m_CHRWritable && addr >= 0x0000 && addr <= 0x1FFF){
        m_CHRROM[addr] = data;
        return true;
    } else if (addr >= 0x2000 && addr <= 0x3EFF) {

        CIRAMWrite(addr, data);
        return  true;
    }

    return false;
}

void Mapper001::drawGUI() {

}