//
// Created by golas on 23.3.23.
//

#ifndef USE_MAPPER001_H
#define USE_MAPPER001_H

#include "Mapper.h"

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
    } CHRMode;

    std::vector<uint8_t> m_CHRROM, m_PRGROM;
    /// Use 32 KB for compatibility among different boards.
    std::array<uint8_t, 0x8000> m_PRGRAM;
    uint8_t m_loadRegister = 0;
    uint8_t m_writeCounter = 0;

    /**
     * MMC1 internal registers.
    */
    struct {

        /// PRG switching mode.
        PRGMode_t PRGMode;
        /// Select PRG ROM bank.
        uint8_t PRGROMSelect;

        /// CHR switching mode.
        CHRMode_t CHRMode;
        /// Select low CHR bank.
        uint8_t CHRROMSelectLow;
        /// Select high CHR bank.
        uint8_t CHRROMSelectHigh;

        /// Enable program RAM.
        bool enablePRGRAM;
        /// Bypass fixed bank mode.
        bool bypassFixedBankLogic;


        void init(){

            /// Default power-on state on most MMC1 mappers. See https://www.nesdev.org/wiki/MMC1.
            PRGMode = PRGMode_t::SWITCH_LOW_FIX_HIGH;
            PRGROMSelect = 0;
            CHRMode = CHRMode_t::SWITCH8KB;
            CHRROMSelectLow = 0;
            CHRROMSelectHigh = 0;
            enablePRGRAM = true;
            bypassFixedBankLogic = false;
        };

    } m_registers;

    void setMirroring(uint8_t rawValue);

public:
    Mapper001(std::vector<uint8_t> & PRGROM, std::vector<uint8_t> & CHRROM);
    ~Mapper001() = default;

    void init() override;
    bool cpuRead(uint16_t addr, uint8_t & data) override;
    bool cpuWrite(uint16_t addr, uint8_t data)  override;
    bool ppuRead(uint16_t addr, uint8_t & data) override;
    bool ppuWrite(uint16_t addr, uint8_t data)  override;
};

#endif //USE_MAPPER001_H
