/**
 * @file Mapper.h
 * @author Ondrej Golasowski (golasowski.o@gmail.com)
 * @brief NES Gamepak mapper circuit common interfrace.
 * @copyright Copyright (c) 2022 Ondrej Golasowski
 *
 */
#ifndef USE_MAPPER_H
#define USE_MAPPER_H

#include "Types.h"

/**
 * "Mapper" base class.
 *
 * Mapper is a name for cartridge boards. Cartridges provided not only the data (program and graphic) but
 * a lot of them also allowed to extend the features of the console. The main ability was bank-switching,
 * which switched data blocks ("mapped" them - that's the origin of the name) and effectively
 * allowed developers to create bigger games.
 *
 * There is a lot of mappers, this class is only a unified interface. A mapper can provide additional
 * storage (e.g. RAM) and functions, but of course needs access to standard data blocks (PRG and CHR).
 * This is emulated by a pointer to the Gamepak class.
 *
 * This class only provides CIRAM. CIRAM is originally located in the PPU and provides
 * a kind of built-in VRAM. Mapper can choose to:
 * a) use CIRAM with fixed mirroring mode (the mode is specified in the header of ROM dump),
 * b) use CIRAM and handle mirroring mode change itself (the mode in ROM dump is then ignored),
 * c) not use CIRAM and handle VRAM itself altogether.
 *
 * @note Mapper is not a standalone component. It is meant to be used with the Gamepak component.
*/
class Mapper {
public:
    /// Type of CIRAM mirroring.
    enum class mirroringType_t {HORIZONTAL, VERTICAL, FOURSCREEN, SINGLE_LO, SINGLE_HI};

protected:
    // ===========================================
    // CIRAM handling
    // ===========================================
    mirroringType_t m_mirroringType = mirroringType_t::HORIZONTAL;
    /// PPU's built-in video memory (VRAM/CIRAM) emulation.
    std::array<uint8_t, 0x800> m_CIRAM {0x00};
    /**
     * Write to CIRAM if used.
     *
     * @param address Address to write to.
     * @param data Data to write.
     * */
    virtual void CIRAMWrite(uint16_t address, uint8_t data);

    /**
     * Read from CIRAM if used.
     *
     * @param address Address to read from.
     * @return Read data.
     * */
    virtual uint8_t CIRAMRead(uint16_t address);

public:
    Mapper() = default;
    virtual ~Mapper() = default;

    /**
     * Initialize mapper to the power-up state = clean all volatile memories.
     * */
    virtual void init();

    /**
     * CPU read interface.
     * @param addr Address to read from.
     * @param data Buffer to write to.
     * @return true If anything was read
     * */
    virtual bool cpuRead(uint16_t addr, uint8_t & data) = 0;

    /**
     * CPU write interface.
     * @param addr Address to write to.
     * @param data Data to write.
     * @return true If anything was written.
     * */
    virtual bool cpuWrite(uint16_t addr, uint8_t data)  = 0;

    /**
     * PPU read interface.
     * @param addr Address to read from.
     * @param data Buffer to write to.
     * @return true If anything was read.
     * */
    virtual bool ppuRead(uint16_t addr, uint8_t & data) = 0;

    /**
     * PPU write interface.
     * @param addr Address to write to.
     * @param data Data to write.
     * @return true If anything was written.
     * */
    virtual bool ppuWrite(uint16_t addr, uint8_t data)  = 0;

    /**
     * Draw a debugging GUI.
     * */
    virtual void drawGUI()                              = 0;
};

#endif //USE_MAPPER_H
