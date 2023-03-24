/**
 * @file Gamepak.h
 * @author Ondrej Golasowski (golasowski.o@gmail.com)
 * @brief NES Gamepak emulation.
 * @copyright Copyright (c) 2022 Ondrej Golasowski
 *
 */

#ifndef USE_GAMEPAK_H
#define USE_GAMEPAK_H

#include <fstream>
#include <istream>
#include <vector>
#include <memory>
#include "Types.h"
#include "Mapper.h"
#include "Port.h"
#include "Component.h"

/**
 * NES/Famicom cartridge emulation class.
 *
 * This class handles parsing of supported cartridge dump formats,
 * loads an appropriate mapper and handles PPU's built in VRAM (CIRAM) if requested.
*/
class Gamepak : public Component {

protected:
    // ===========================================
    // Constants
    // ===========================================
    /// Size of program ROM unit.
    static const size_t PRGROM_UNIT_SIZE = 16384;
    /// Size of character ROM unit.
    static const size_t CHRROM_UNIT_SIZE = 8192;
    /// Size of program RAM unit.
    static const size_t PRGRAM_UNIT_SIZE = 8192;

    // ===========================================
    // Types
    // ===========================================
    /// Type of CIRAM mirroring.
    enum class mirroringType_t {HORIZONTAL, VERTICAL, FOURSCREEN, SINGLE_LO, SINGLE_HI};
    /// Type of file format.
    enum class fileFormat_t {NES20, INES, ARCHAICINES};
    /// Console type.
    enum class consoleType_t {STANDARD, VSUNISYSTEM, PLAYCHOICE, EXTENDED};
    /// TV region type.
    enum class tvSystem_t {NTSC, PAL, MULTI, DENDY};

    // ===========================================
    // Parameters
    // ===========================================
    /// Gamepak parameters.
    struct gamepakParams{

        // Common for all formats.
        /// Absolute size of PRG ROM.
        size_t PRGROMsize;
        /// Absolute size of CHR ROM.
        size_t CHRROMsize;
        /// Mirroring type.
        mirroringType_t mirroringType;
        /// File format.
        fileFormat_t fileFormat;
        /// Whether the gamepak contains any persistent memory.
        bool hasPersistentMemory;
        /// Whether the gamepak contains trainer.
        bool hasTrainer;
        /// iNES mapper number.
        uint16_t mapperNumber;

        // iNES + NES 2.0
        /// Absolute size of PRG RAM.
        size_t PRGRAMsize;
        /// Console type.
        consoleType_t consoleType;
        /// TV region type.
        tvSystem_t tvSystem;

        // NES 2.0
        /// Mapper number specifier.
        uint8_t submapperNumber;
        /// Non-volatile PRG memory absolute size.
        size_t PRGNVRAMsize;
        /// Character RAM absolute size.
        size_t CHRRAMsize;
        /// Character NVRAM absolute size.
        size_t CHRNVRAMsize;

        gamepakParams() {
            init();
        }

        /// Init parameters to common default values for all types.
        void init() {
            PRGROMsize = 0;
            CHRROMsize = 0;
            mirroringType = mirroringType_t::HORIZONTAL;
            fileFormat = fileFormat_t::INES;
            hasPersistentMemory = false;
            hasTrainer = false;
            mapperNumber = 0;
            PRGRAMsize = 0;
            consoleType = consoleType_t::STANDARD;
            tvSystem = tvSystem_t::NTSC;
            submapperNumber = 0;
            PRGNVRAMsize = 0;
            CHRRAMsize = 0;
            CHRNVRAMsize = 0;
        }

    } m_params;

    // ===========================================
    // Data
    // ===========================================

    /// Trainer data.
    std::vector<uint8_t> m_trainer;
    /// Program data.
    std::vector<uint8_t> m_PRGROM;
    /// Character (picture) data.
    std::vector<uint8_t> m_CHRROM;

//    std::vector<uint8_t> m_INSTROM;
//    std::vector<uint8_t> m_PROM;

    // ===========================================
    // Mapper handling
    // ===========================================
    /// Gamepak's mapper.
    std::unique_ptr<Mapper> m_mapper;

    // ===========================================
    // CIRAM handling
    // ===========================================
    /// PPU's built-in video memory (VRAM/CIRAM) emulation.

    std::array<uint8_t, 0x800> m_CIRAM {0x00};
    /**
     * Write to CIRAM if used.
     *
     * @param address Address to write to.
     * @param data Data to write.
     * */
    void CIRAMWrite(uint16_t address, uint8_t data);

    /**
     * Read from CIRAM if used.
     *
     * @param address Address to read from.
     * @return Read data.
     * */
    uint8_t CIRAMRead(uint16_t address);

public:
    Gamepak();
    ~Gamepak() override;

    /**
     * Initialize Gamepak to power-up state.
     *
     * @note This does not delete any previously loaded ROM, only deletes data stored on volatile memories. */
    void init() override;

    /**
     * Load a ROM file.
     *
     * @param ifs Input stream containing ROM data.
     * */
    void load(std::ifstream & ifs);

    /**
     * Render a debugging GUI.
     * Shows Gamepak parameters and mapper's internal state.
     *
     * @return GUI windows.
     * */
    std::vector<EmulatorWindow> getGUIs() override;

};

#endif //USE_GAMEPAK_H
