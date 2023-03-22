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
*/
class Mapper {
public:
    Mapper() = default;
    virtual ~Mapper() = default;

    /**
     * Initialize mapper to the power-up state = clean all volatile memories.
     * */
    virtual void init()                                 = 0;

    /**
     * Whether to use PPU's built in video memory.
     * @return true If the mapper wants to use CIRAM.
     * */
    virtual bool useCIRAM()                             = 0;

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
