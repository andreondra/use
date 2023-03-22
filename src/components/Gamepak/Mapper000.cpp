/**
 * @file Mapper000.cpp
 * @author Ondrej Golasowski (golasowski.o@gmail.com)
 * @brief iNES Mapper 000 implementation.
 * @copyright Copyright (c) 2023 Ondrej Golasowski
 *
 */

#include <stdexcept>
#include "imgui.h"
#include "components/Gamepak/Mapper000.h"

Mapper000::Mapper000(std::vector<uint8_t> &PRGROM, std::vector<uint8_t> &CHRROM)
    : m_PRGROM(PRGROM), m_CHRROM(CHRROM) {

    // Only 16 KiB or 32 KiB allowed.
    if(
        m_PRGROM.size() != 0x4000 &&
        m_PRGROM.size() != 0x8000
    ) {
        throw std::invalid_argument("NROM supports only either 16 KiB or 32 KiB of program ROM.");
    }

    // Empty CHR ROM = there is no ROM, use CHR RAM.
    if(m_CHRROM.empty()) {
        m_CHRRAM.resize(0x2000, 0x0);
    // If there is CHR ROM, its size has to be 0x2000.
    } else if (m_CHRROM.size() != 0x2000) {
        throw std::invalid_argument("NROM expects 8 KiB of character ROM.");
    }
}

void Mapper000::init() {
    m_PRGRAM.fill(0x00);
    std::fill(m_CHRRAM.begin(), m_CHRRAM.end(), 0x0);
}

bool Mapper000::useCIRAM() {
    return false;
}

bool Mapper000::cpuRead(uint16_t addr, uint8_t &data) {

    if(addr >= 0x6000 && addr <= 0x7FFF){
        data = m_PRGRAM[addr & (m_PRGRAM.size() - 1)];
        return true;
    } else if(addr >= 0x8000 && addr <= 0xFFFF){
        data = m_PRGROM[addr & (m_PRGROM.size() - 1)];
        return true;
    }

    return false;
}

bool Mapper000::cpuWrite(uint16_t addr, uint8_t data) {

    if(addr >= 0x6000 && addr <= 0x7FFF){
        m_PRGRAM[addr & (m_PRGRAM.size() - 1)] = data;
        return true;
    } else if(addr >= 0x8000 && addr <= 0xFFFF){
        m_PRGROM[addr & (m_PRGROM.size() - 1)] = data;
        return true;
    }

    return false;
}

bool Mapper000::ppuRead(uint16_t addr, uint8_t &data) {

    if(addr >= 0x0000 && addr <= 0x1FFF) {

        // No ROM, use built-in RAM.
        if(m_CHRROM.empty())
            data = m_CHRRAM[addr];
        else
            data = m_CHRROM[addr];

        return true;
    }

    return false;
}

bool Mapper000::ppuWrite(uint16_t addr, uint8_t data) {

    if(addr >= 0x0000 && addr <= 0x1FFF) {

        // No ROM, use built-in RAM.
        if(m_CHRROM.empty())
            m_CHRRAM[addr] = data;
        else
            m_CHRROM[addr] = data;

        return true;
    }

    return false;
}

void Mapper000::drawGUI() {

    ImGui::Text("Type: iNES 000 (NROM)");
    if(m_PRGROM.size() == 0x8000) {
        ImGui::Text("Subtype: NROM-256");
    } else {
        ImGui::Text("Subtype: NROM-128");
    }
}