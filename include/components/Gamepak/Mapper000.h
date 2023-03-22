/**
 * @file Mapper000.h
 * @author Ondrej Golasowski (golasowski.o@gmail.com)
 * @brief iNES Mapper 000.
 * This mapper is used in boards NROM, HROM, RROM, RTROM, SROM, STROM.
 * @copyright Copyright (c) 2023 Ondrej Golasowski
 *
 */

#ifndef USE_MAPPER000_H
#define USE_MAPPER000_H

#include <vector>
#include <array>
#include <cstdint>
#include "Types.h"
#include "Mapper.h"

/**
 * iNES mapper 000.
 *
 * PRG ROM: 16 or 32 KiB.
 * PRG RAM: 2 or 4 KiB, this implementation provides always 8 KiB for compatibility.
 * CHR ROM: 8 KiB
 * Mirroring settings: via solder pads.
 * This mapper has no bankswitching support.
 * */
class Mapper000 : public Mapper {
protected:
    std::vector<uint8_t> & m_PRGROM;
    std::vector<uint8_t> & m_CHRROM;

    std::array<uint8_t, 0x2000> m_PRGRAM{0x00};
    std::vector<uint8_t> m_CHRRAM;

public:
    /**
     * Construct Mapper 000.
     *
     * @throw std::invalid_argument If PRGROM has invalid size (not 0x4000 nor 0x8000).
     * @throw std::invalid_argument If CHRROM has invalid size (not 0x8000).
     * */
    Mapper000(std::vector<uint8_t> & PRGROM, std::vector<uint8_t> & CHRROM);
    ~Mapper000() override = default;

    void init() override;
    bool useCIRAM() override;
    bool cpuRead(uint16_t addr, uint8_t & data) override;
    bool cpuWrite(uint16_t addr, uint8_t data)  override;
    bool ppuRead(uint16_t addr, uint8_t & data) override;
    bool ppuWrite(uint16_t addr, uint8_t data)  override;

    void drawGUI() override;

};

#endif //USE_MAPPER000_H
