/**
 * @file 2C02.h
 * @author Ondrej Golasowski (golasowski.o@gmail.com)
 * @brief Ricoh 2C02 (PPU) software implementation.
 * @copyright Copyright (c) 2023 Ondrej Golasowski
 *
 */

#ifndef USE_2C02_H
#define USE_2C02_H

#include <cstdint>
#include <cstring>
#include <vector>
#include "Port.h"
#include "Component.h"

/**
 *
 *
*/
class R2C02 : public Component {

// Notes.
/**
 * Frame rendering:
 *                  256   341
 *      xxxxxxxxxxxxxxx   <- scanline -1
 *      @@@@@@@@@@@@@@@---
 *      @@@@@@@@@@@@@@@---
 *      @@@@@@@@@@@@@@@---
 *      N@@@@@@@@@@@@@@--- 240
 *      ---------------
 *      ---------------    261 (vblank)
 * N = NMI trigger
 * @ = visible screen
 * - = invisible screen
*/


public:
    /**
     * Utility struct to carry RGB values.
    */
    struct RGB_t{
        uint8_t red;
        uint8_t green;
        uint8_t blue;
    };

    static const uint16_t NESSCREENWIDTH = 256;
    static const uint16_t NESSCREENHEIGHT = 240;

    uint8_t m_dataBuffer = 0x00;

    /**
     * Object attribute memory.
     * OAM is used to store sprite data.
    */
    // union OAM_t{
    //     struct STR{
    //         uint8_t y;          //!< Y position of the top side.
    //         uint8_t tileIndex;  //!< Tile index. See wiki.
    //         uint8_t attributes; //!< Tile attributes.
    //         uint8_t x;          //!< X position of the left side.
    //     } bits;
    //     uint32_t data;
    // } m_OAM[64];

    /**
     * Internal registers of the PPU for the rendering purposes.
     * These are not exposed.
    */
    struct internalRegisters_t{

        /**
         * v = current VRAM address
         * t = temporary VRAM address
        */
        union{

            struct STR{

                uint16_t coarseX : 5;
                uint16_t coarseY : 5;
                uint16_t nameX : 1;
                uint16_t nameY : 1;
                uint16_t fineY : 3;
                uint16_t padding : 1;
            } bits;
            uint16_t data;
        } v,t;

        uint8_t x;  //!< Fine X scroll. 3 bits.
        bool w;     //!< First or second write toggle. Shared by 0x2005 and 0x2006 register. 1 bit.

    } m_internalRegisters;

    struct{

        // 64 sprites for the frame.
        uint8_t primaryOAM[256];
        // 8 sprites for the current scanline.
        uint8_t secondaryOAM[32];
        // Attributes.
        uint8_t attrLatch[8];
        // X positions.
        uint8_t x[8];

        /**
         * 8 pairs of 8 bit shifters
         * Pattern table data for up to 8 sprites to be rendered on the current scanline.
        */
        uint8_t shiftLo[8];
        uint8_t shiftHi[8];
        uint8_t allowShift[8];

        uint8_t secondarySpriteId;

        uint8_t feedY;
        uint16_t feedTileAddress;
        uint8_t feedIndex;

        void renderInit(){

            memset(secondaryOAM, 0xFF, 32);

            secondarySpriteId = 0;
            feedIndex = 0;
        }

        void shiftClear(){

            memset(x, 0, 8);
            memset(shiftHi, 0, 8);
            memset(shiftLo, 0, 8);
            memset(attrLatch, 0, 8);
            memset(allowShift, 0, 8);
        }

        void shift(){

            for(uint8_t i = 0; i < 8; i++)
                if(allowShift[i]){
                    if((attrLatch[i] & 0x40) > 0){

                        shiftHi[i] >>= 1;
                        shiftLo[i] >>= 1;
                    } else {

                        shiftHi[i] <<= 1;
                        shiftLo[i] <<= 1;
                    }
                }
        }

        void clear(){

            memset(primaryOAM, 0, 256);
            shiftClear();
            renderInit();
        }
    }m_spriteData;

    struct{

        uint8_t ntByte;
        uint8_t atByte;
        uint16_t tileData;

        uint16_t shiftTileLo;
        uint16_t shiftTileHi;
        uint16_t shiftAttrLo;
        uint16_t shiftAttrHi;
    } m_backgroundData;

    struct registers_t{

        /**
         * Controller (0x2000) - PPUCTRL
         * Access: write
         * Controls PPU operation.
        */
        union{

            struct{
                uint8_t nameX : 1;              //!< Base nametable address, scroll X coordinate MSB. (1 = add 256)
                uint8_t nameY : 1;              //!< Base nametable address, scroll Y coordinate MSB. (1 = add 240)
                uint8_t incMode : 1;            //!< VRAM address increment per CPU R/W of PPUDATA. (0 = add 1, 1 = add 32).
                uint8_t spriteAddress : 1;      //!< Sprite pattern table address for 8x8 sprites.
                uint8_t backgroundAddress : 1;  //!< Background pattern table address.
                uint8_t spriteSize : 1;         //!< Sprite size (8x8, 8x16).
                uint8_t masterSlave : 1;        //!< PPU master/slave.
                uint8_t nmi : 1;                //!< Generate NMI on vblank.
            } bits;

            uint8_t data;
        } ppuctrl;

        /**
         * Mask register (0x2001) - PPUMASK
         * Access: write
         * Controls the rendering of sprites and backgrounds and color effects.
        */
        union{

            struct{
                uint8_t grayscale : 1;          //!< Enable grayscale.
                uint8_t showBackgroundLeft : 1; //!< Show background in leftmost 8 pixels.
                uint8_t showSpritesLeft : 1;    //!< Show sprites in leftmost 8 pixels.
                uint8_t showBackground : 1;     //!< Show bg.
                uint8_t showSprites : 1;        //!< Show sprites.
                uint8_t eRed : 1;               //!< Enhance red. (Green on PAL/Dendy.)
                uint8_t eGreen : 1;             //!< Enhance green. (Red on PAL/Dendy.)
                uint8_t eBlue : 1;              //!< Enhance blue.

            } bits;

            uint8_t data;
        } ppumask;

        /**
         * Status register (0x2002) - PPUSTATUS
         * Access: read
         * Reflects the state of the PPU.
        */
        union{

            struct{
                uint8_t padding : 5;        //!< Least sig. bits of previous value written to PPU register. Some Vs. systems return a constant that game checks.
                uint8_t spriteOverflow : 1; //!< Buggy detection of the existence of more than 8 sprites on a scanline.
                uint8_t spriteZeroHit : 1;  //!< Nonzero pixel of sprite 0 overlaps nonzero background.
                uint8_t vBlank : 1;         //!< True if vblank started, cleared after reading this register.
            } bits;

            uint8_t data;
        } ppustatus;

        union{

            struct{
                uint8_t byte : 2;
                uint8_t index : 6;
            } bits;

            uint8_t data;
        } oamAddress;

    } m_registers;

    uint8_t m_nametables[2048];
    int m_clock, m_scanline;

private:

    bool m_scanlineReady = false;
    bool m_frameReady = false;
    bool m_oddScan = false;

    /**
     * Suppress NMI generation and NMI flag setting.
     * Used by a special case during reading NMI flag near the clock 1 of the scanline 241,
     * which is when the NMI is supposed to generate.
     * See read method for more information.
    */
    bool m_blockNMI = false;

    /**
     * Default 2C02 color pallete.
    */
    const RGB_t m_colors2C02[64] = {

            {84, 84, 84},
            {0, 30, 116},
            {8, 16, 144},
            {48, 0, 136},
            {68, 0, 100},
            {92, 0, 48},
            {84, 4, 0},
            {60, 24, 0},
            {32, 42, 0},
            {8, 58, 0},
            {0, 64, 0},
            {0, 60, 0},
            {0, 50, 60},
            {0, 0, 0},
            {0, 0, 0},
            {0, 0, 0},
            {152, 150, 152},
            {8, 76, 196},
            {48, 50, 236},
            {92, 30, 228},
            {136, 20, 176},
            {160, 20, 100},
            {152, 34, 32},
            {120, 60, 0},
            {84, 90, 0},
            {40, 114, 0},
            {8, 124, 0},
            {0, 118, 40},
            {0, 102, 120},
            {0, 0, 0},
            {0, 0, 0},
            {0, 0, 0},
            {236, 238, 236},
            {76, 154, 236},
            {120, 124, 236},
            {176, 98, 236},
            {228, 84, 236},
            {236, 88, 180},
            {236, 106, 100},
            {212, 136, 32},
            {160, 170, 0},
            {116, 196, 0},
            {76, 208, 32},
            {56, 204, 108},
            {56, 180, 204},
            {60, 60, 60},
            {0, 0, 0},
            {0, 0, 0},
            {236, 238, 236},
            {168, 204, 236},
            {188, 188, 236},
            {212, 178, 236},
            {236, 174, 236},
            {236, 174, 212},
            {236, 180, 176},
            {228, 196, 144},
            {204, 210, 120},
            {180, 222, 120},
            {168, 226, 144},
            {152, 226, 180},
            {160, 214, 228},
            {160, 162, 160},
            {0, 0, 0},
            {0, 0, 0}
    };

    //External PPU colors.

    // Current PPU rendering colors.
    const RGB_t *m_colors = m_colors2C02;

    // ===============================================
    // Background rendering procedures.
    void verticalIncrement();
    void horizontalIncrement();
    void verticalTransfer();
    void horizontalTransfer();
    void fetchNT();
    void fetchAT();
    void fetchTileLo();
    void fetchTileHi();
    void feedShifters();
    void shiftShifters();
    // ===============================================
    // Foreground rendering procedures.
    void evaluateSprites();
    void fetchSprite(uint8_t offset);
    // ===============================================
    /**
     * 2C02 bus components:
     * 0x0000 - 0x1FFF Pattern memory (CHR ROM) - on cartridge (sprites)
     * 0x2000 - 0x3EFF Nametable memory (VRAM) - on cartridge or built-in (sprite locations)
     * 0x3F00 - 0x3FFF Palette memory - built-in (colors)
    */
    // 2 KiB of built-in VRAM.
    //uint8_t m_nametables[2048]; // moved to public
    // Color palletes.
    uint8_t m_palettes[32];
    // ===============================================
    // Current rendering coordinates.
    //int m_clock, m_scanline; // public
    // ===============================================
    // NES screen.
    RGB_t m_screen[NESSCREENWIDTH][NESSCREENHEIGHT] = {0,0,0};

    // Internal PPU bus I/O.
    /**
     * Read from the internal bus.
     * @param addr Address to read from.
     * @return Data.
    */
    uint8_t ppuBusRead(uint16_t addr);
    /**
     * Write to the internal bus.
     * @param addr Address to write to.
     * @param data Data to write.
    */
    void ppuBusWrite(uint16_t addr, uint8_t data);
    // ===============================================
    RGB_t getPixelColor(uint8_t paletteId, uint8_t pixel);
    uint8_t saturate(uint8_t x, uint8_t y);
    uint8_t desaturate(uint8_t x, uint8_t y);

    // ===========================================
    // I/O
    // ===========================================
    // Connection to the PPU bus (controlled by the PPU).
    DataPort m_ppuBus;
    // Interrupt generator. Normally connected to the 6502's NMI pin.
    SignalPort m_INT;

public:

    /**
     * Default constructor.
     * Reset PPU status and rendering.
    */
    R2C02();
    /**
     * Default destructor.
    */
    ~R2C02() = default;

    // ===============================================
    // PPU control.
    /**
     * Reset the PPU.
    */
    void init();

    /**
     * Proceed one clock further in emulation.
     * Works as a physical CLK input.
     * 1 PPU cycle = 186 ns
    */
    void clock();

    // ===============================================
    /**
     * PPU OAM DMA.
     * Used to write to the OAM during DMA proccess.
     * @param addr OAM address.
     * @param data Data to write.
    */
    void OAMDMA(uint8_t addr, uint8_t data);
    // ===============================================
    // Output
    std::vector<RGB_t> getPalette(uint8_t paletteId);
    uint8_t *getPaletteRAM();
    /**
     * Get a pattern table.
    */
    std::vector<RGB_t> getPatternTable(uint8_t paletteId, uint8_t index);
    /**
     * Get a NES screen.
    */
    std::vector<RGB_t> getScreen();
    /**
     * Is a scanline finished?
     * @return true if clock index >= 341
    */
    bool scanlineFinished() const;
    /**
     * Is a frame finished?
     * @return true if scanline index >= 261
    */
    bool frameFinished() const;

    std::vector<EmulatorWindow> getGUIs() override;

};

#endif //USE_2C02_H
