#include "components/Gamepak/Mapper001.h"
#include "Types.h"

Mapper001::Mapper001(std::vector<uint8_t> & PRGROM, std::vector<uint8_t> & CHRROM)
        : m_PRGROM(PRGROM), m_CHRROM(CHRROM) {
}

void Mapper001::setMirroring(uint8_t rawValue) {
    switch(rawValue) {
        case 0: m_mirroringType = mirroringType_t::SINGLE_LO;
        case 1: m_mirroringType = mirroringType_t::SINGLE_HI;
        case 2: m_mirroringType = mirroringType_t::VERTICAL;
        case 3: m_mirroringType = mirroringType_t::HORIZONTAL;
    }
}

void Mapper001::init(){

    Mapper::init();
    m_registers.init();
    m_PRGRAM.fill(0x00);

    m_loadRegister = 0;
    m_writeCounter = 0;
}

bool Mapper001::cpuRead(uint16_t addr, uint8_t & data) {

    // PRG RAM bank.
    if(addr >= 0x6000 && addr <= 0x7FFF){

        data = m_PRGRAM.at(addr & 0x1FFF);
        return true;
        // PRG ROM bank 0.
    } else if(addr >= 0x8000 && addr <= 0xBFFF) {

        // Switch 32 KB, ignore low bit of PRG bank number.
        if(m_registers.m_control.PRGmode == 0 || m_registers.m_control.PRGmode == 1){
            data = m_PRGROM->at(addr - 0x8000 + (m_registers.m_PRGbank.select & 0x1E) * 16384);

            // Fixed to the first bank.
        } else if(m_registers.m_control.PRGmode == 2){
            data = m_PRGROM->at(addr - 0x8000);

            // Switch 16 KB.
        } else if(m_registers.m_control.PRGmode == 3){
            data = m_PRGROM->at((addr & 0x3FFF) + m_registers.m_PRGbank.select * 16384);
        }

        return true;
    } else if(addr >= 0xC000 && addr <= 0xFFFF) {

        // Switch 32 KB, ignore low bit of PRG bank number.
        if(m_registers.m_control.PRGmode == 0 || m_registers.m_control.PRGmode == 1){
            data = m_PRGROM->at(addr - 0x8000 + (m_registers.m_PRGbank.select & 0x1E) * 16384);

            // Switch 16 KB.
        } else if(m_registers.m_control.PRGmode == 2){
            data = m_PRGROM->at((addr & 0x3FFF) + m_registers.m_PRGbank.select * 16384);

            // Fixed to the last bank.
        } else if(m_registers.m_control.PRGmode == 3){
            data = m_PRGROM->at((addr & 0x3FFF) + (m_meta->PRGROMunits - 1) * 16384);
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

        m_PRGRAM[addr & 0x1FFF] = data;
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
            m_registers.m_control.PRGmode = 3;
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

                    // CHR 0 bank.
                    case 1:
                        m_registers.CHRROMSelectLow = m_loadRegister & 0x1F;

                        m_registers.m_CHR0bank.PRGRAMselect = (m_loadRegister & 0xC) >> 2;
                        m_registers.m_PRGbank.select &= 0xF;
                        m_registers.m_PRGbank.select |= m_loadRegister & 0x10;
                        break;

                        // CHR 1 bank.
                    case 2:
                        m_registers.CHRROMSelectHigh = m_loadRegister & 0x1F;

                        m_registers.m_CHR1bank.PRGRAMselect = (m_loadRegister & 0xC) >> 2;
                        if(m_registers.m_control.CHRmode == 0){
                            m_registers.m_PRGbank.select &= 0xF;
                            m_registers.m_PRGbank.select |= m_loadRegister & 0x10;
                        }
                        break;

                        // PRG bank.
                    case 3:
                        m_registers.m_PRGbank.select &= 0x10;
                        m_registers.m_PRGbank.select |= m_loadRegister & 0xF;
                        m_registers.m_PRGbank.enableRAM = m_loadRegister & 0x10;
                        break;
                }

                // Avoiding index overflow in a case of a bad program.
                m_registers.m_PRGbank.select %= m_meta->PRGROMunits;

                if(m_meta->CHRROMunits > 0){
                    m_registers.m_CHR0bank.CHRROMselect %= m_meta->CHRROMunits;
                    m_registers.m_CHR1bank.CHRROMselect %= m_meta->CHRROMunits;
                }

                m_writeCounter = 0;
                m_loadRegister = 0;
            }
        }


        return true;
    }

    return false;
}

bool Mapper001::ppuRead(uint16_t addr, uint8_t & data){

    if(m_meta->CHRROMunits == 0){

        if(addr >= 0x0000 && addr <= 0x1FFF){
            data = m_CHRRAM.at(addr);
            return true;
        }

    } else {

        if(addr >= 0x0000 && addr <= 0x0FFF){

            // Switch two separate 4 KB banks.
            if(m_registers.m_control.CHRmode == 1){

                data = m_CHRROM->at(addr + m_registers.m_CHR0bank.CHRROMselect * 4096);
                // Switch 8 KB.
            } else {
                data = m_CHRROM->at(addr + (m_registers.m_CHR0bank.CHRROMselect & 0x1E) * 8192);
            }
            return true;
        } else if(addr >= 0x1000 && addr <= 0x1FFF){

            // Switch two separate 4 KB banks.
            if(m_registers.m_control.CHRmode == 1){

                data = m_CHRROM->at((addr & 0xFFF) + m_registers.m_CHR1bank.CHRROMselect * 4096);
                // Switch 8 KB.
            } else {
                data = m_CHRROM->at(addr + (m_registers.m_CHR0bank.CHRROMselect & 0x1E) * 8192);
            }
            return true;
        }
    }

    return false;
}

bool Mapper001::ppuWrite(uint16_t addr, uint8_t data){

    if(m_meta->CHRROMunits == 0){

        if(addr >= 0x0000 && addr <= 0x1FFF){
            m_CHRRAM.at(addr) = data;
            return true;
        }

    }
    return false;
}