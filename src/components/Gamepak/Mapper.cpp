/**
 * @file Mapper.cpp
 * @author Ondrej Golasowski (golasowski.o@gmail.com)
 * @brief NES mapper implementation.
 * @copyright Copyright (c) 2022 Ondrej Golasowski
 *
 */
#include "components/Gamepak/Mapper.h"

void Mapper::init() {
    m_CIRAM.fill(0x00);
}

uint8_t Mapper::CIRAMRead(uint16_t address) {

    // We are interested in the bottom 12 bits, because the nametable addr range is (partially) mirrored.
    // 0x2000 - 0x23FF = NT 1
    // 0x2400 - 0x27FF = NT 2
    // 0x2800 - 0x2BFF = NT 3
    // 0x2C00 - 0x2FFF = NT 4
    // 0x3000 - 0x3EFF = Mirrors of 0x2000 - 0x2EFF.
    //
    // NT indexes:
    // 1 | 2
    // --+--
    // 3 | 4
    address &= 0xFFF;

    // Horizontal mirroring
    // A | A
    // --+--
    // B | B
    if(m_mirroringType == mirroringType_t::HORIZONTAL){

        if(address >= 0x000 && address <= 0x3FF)
            return m_CIRAM[address & 0x3FF];
        else if(address >= 0x400 && address <= 0x7FF)
            return m_CIRAM[address & 0x3FF];
        else if(address >= 0x800 && address <= 0xBFF)
            return m_CIRAM[0x400 + (address & 0x3FF)];
        else if(address >= 0xC00 && address <= 0xFFF)
            return m_CIRAM[0x400 + (address & 0x3FF)];

        // Vertical mirroring
        // A | B
        // --+--
        // A | B
    } else if(m_mirroringType == mirroringType_t::VERTICAL) {

        if(address >= 0x000 && address <= 0x3FF)
            return m_CIRAM[address & 0x3FF];
        else if(address >= 0x400 && address <= 0x7FF)
            return m_CIRAM[0x400 + (address & 0x3FF)];
        else if(address >= 0x800 && address <= 0xBFF)
            return m_CIRAM[address & 0x3FF];
        else if(address >= 0xC00 && address <= 0xFFF)
            return m_CIRAM[0x400 + (address & 0x3FF)];

        // One screen low.
        // A | A
        // --+--
        // A | A
    } else if(m_mirroringType == mirroringType_t::SINGLE_LO){
        return m_CIRAM[address & 0x3FF];

        // One screen high.
        // B | B
        // --+--
        // B | B
    } else if(m_mirroringType == mirroringType_t::SINGLE_HI){
        return m_CIRAM[0x400 + (address & 0x3FF)];
    }

    return 0x00;
}

void Mapper::CIRAMWrite(uint16_t address, uint8_t data) {

    address &= 0xFFF;

    // Horizontal mirroring.
    if(m_mirroringType == mirroringType_t::HORIZONTAL){

        if(address >= 0x000 && address <= 0x3FF)
            m_CIRAM[address & 0x3FF] = data;
        else if(address >= 0x400 && address <= 0x7FF)
            m_CIRAM[address & 0x3FF] = data;
        else if(address >= 0x800 && address <= 0xBFF)
            m_CIRAM[0x400 + (address & 0x3FF)] = data;
        else if(address >= 0xC00 && address <= 0xFFF)
            m_CIRAM[0x400 + (address & 0x3FF)]  = data;

        // Vertical mirroring.
    } else if(m_mirroringType == mirroringType_t::VERTICAL) {

        if(address >= 0x000 && address <= 0x3FF)
            m_CIRAM[address & 0x3FF] = data;
        else if(address >= 0x400 && address <= 0x7FF)
            m_CIRAM[0x400 + (address & 0x3FF)] = data;
        else if(address >= 0x800 && address <= 0xBFF)
            m_CIRAM[address & 0x3FF] = data;
        else if(address >= 0xC00 && address <= 0xFFF)
            m_CIRAM[0x400 + (address & 0x3FF)] = data;

        // Only low NT used.
    } else if(m_mirroringType == mirroringType_t::SINGLE_LO){
        m_CIRAM[address & 0x3FF] = data;

        // Only high NT used.
    } else if(m_mirroringType == mirroringType_t::SINGLE_HI){
        m_CIRAM[0x400 + (address & 0x3FF)] = data;
    }
}