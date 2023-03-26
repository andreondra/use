/**
 * @file Mapper001.h
 * @author Ondrej Golasowski (golasowski.o@gmail.com)
 * @brief iNES Mapper 001.
 * This mapper is used in boards SKROM, SLROM, SNROM...
 * @copyright Copyright (c) 2023 Ondrej Golasowski
 */


#ifndef USE_MAPPER001_H
#define USE_MAPPER001_H

#include "Mapper.h"

/**
 * iNES mapper 001.
 *
 * PRG ROM: 256 or 512 KiB.
 * PRG RAM: 8-32 KiB (default 32 KiB, selectable only with NES 2.0 dumps).
 * CHR ROM: 8-128 KiB
 *
 * If no CHR ROM present (0 KiB), m_CHRRAM will be mapped to CHR ROM and 8 KiB of memory provided.
 *
 * Mirroring settings: H, V, single or switchable.
 * Both PRG and CHR ROMs support banking.
 * */
class Mapper001 : public Mapper{

private:

    // Custom types.
    /**
     * Mode of the program ROM bank switching.
     * HW values are mapped to enums for easy identification.
     * */
    enum class PRGMode_t : uint8_t {
        /// Switch both banks (32 KB).
        SWITCH_BOTH0 = 0,
        /// Switch both banks (32 KB) -- same as previous.
        SWITCH_BOTH1 = 1,
        /// Switch only high bank (16 KB), fix the low to the first bank.
        FIX_LOW_SWITCH_HIGH = 2,
        /// Switch only low bank (16 KB), fix the high to the last bank.
        SWITCH_LOW_FIX_HIGH = 3
    };

    /**
     * Mode of character ROM bank switching.*/
    enum class CHRMode_t : uint8_t {
        /// Switch by 8 KB
        SWITCH8KB = 0,
        /// Switch by 4 KB
        SWITCH4KB = 1
    };

    std::vector<uint8_t> & m_CHRROM;
    std::vector<uint8_t> & m_PRGROM;
    std::vector<uint8_t> m_CHRRAM;
    bool m_CHRWritable = false;

    std::vector<uint8_t> m_PRGRAM;
    /// Shift register accessible via serial port at $8000-$FFFF.
    uint8_t m_loadRegister = 0;
    /// Count of writes to the shift register.
    uint8_t m_writeCounter = 0;

    /**
     * Mapper config registers.
    */
    struct {

        /// PRG switching mode.
        PRGMode_t PRGMode;
        /// Select PRG ROM bank.
        uint8_t PRGROMSelect;

        /// CHR switching mode.
        CHRMode_t CHRMode;
        /// Select low CHR bank.
        uint8_t CHRROMLoSelect;
        /// Select high CHR bank.
        uint8_t CHRROMHiSelect;

        /// Enable program RAM.
        bool enablePRGRAM;
        /// PRG RAM bank select (A13 for 16 KB version; A14-A13 for 32 KB version).
        uint8_t PRGRAMSelect;

        /// Initialize mapper to the power-on state.
        void init(){

            /// Default power-on state on most MMC1 mappers. See https://www.nesdev.org/wiki/MMC1.
            PRGMode = PRGMode_t::SWITCH_LOW_FIX_HIGH;
            PRGROMSelect = 0;
            CHRMode = CHRMode_t::SWITCH8KB;
            CHRROMLoSelect = 0;
            CHRROMHiSelect = 0;
            enablePRGRAM = true;
        };

    } m_registers;

    void setMirroring(uint8_t rawValue);

public:
    /**
     * Create instance of Mapper 001.
     *
     * PRG RAM size can be selected in NES 2.0 headers, default of 32 KiB is provided for
     * backwards compatibility.
     * */
    Mapper001(std::vector<uint8_t> & PRGROM, std::vector<uint8_t> & CHRROM, size_t PRGRAMSize = 0x8000);
    ~Mapper001() override = default;

    void init() override;
    bool cpuRead(uint16_t addr, uint8_t & data) override;
    bool cpuWrite(uint16_t addr, uint8_t data)  override;
    bool ppuRead(uint16_t addr, uint8_t & data) override;
    bool ppuWrite(uint16_t addr, uint8_t data)  override;

    void drawGUI() override;
};

#endif //USE_MAPPER001_H
