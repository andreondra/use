/**
 * @file 2C02.cpp
 * @author Ondrej Golasowski (golasowski.o@gmail.com)
 * @brief Ricoh 2C02 (PPU) software implementation.
 * @copyright Copyright (c) 2023 Ondrej Golasowski
 *
 */

#include "components/2C02.h"
#include <cstring>
#include "imgui.h"

R2C02::R2C02() {

    m_connectors["cpuBus"] = std::make_shared<Connector>(DataInterface{

            .read = [&](uint32_t address, uint32_t & buffer) -> bool {

                // Check whether address is inside the PPU address space.
                // OAMDMA register.
                if(address == 0x4014) {
                    // Halt CPU
                    // Copy from specified memory
                // 8 remaining registers are mirrored across the whole space -> ANDing with 0x7.
                } else if(address >= 0x2000 && address <= 0x3FFF) {
                    address &= 0x7;
                // Not mappped.
                } else {
                    return false;
                }

                switch(address){

                    // PPUSTATUS. Read only.
                    case 0x0002:

                        // Normally, the NMI is generated at clock 1, scanline 241.
                        // However, if the CPU reads the status register one clock
                        // before and during the NMI generation clock, the behaviour is
                        // changed.
                        if(m_scanline == 241){

                            // One clock before NMI generation clock.
                            // Vertical blank flag is read as false, NMI is blocked.
                            if(m_clock == 0){
                                m_registers.ppustatus.bits.vBlank = 0;
                                m_blockNMI = true;
                            }

                            // NMI generation clock.
                            // Vertical blank flag is read as true but NMI is blocked.
                            else if(m_clock == 1){
                                m_registers.ppustatus.bits.vBlank = 1;
                                m_blockNMI = true;
                            }
                        }

                        buffer = (m_registers.ppustatus.data & 0xE0) | (m_dataBuffer & 0x1F);
                        m_registers.ppustatus.bits.vBlank = 0;
                        m_internalRegisters.w = false;
                        break;

                    // OAM data.
                    case 0x0004:
                        buffer = m_spriteData.primaryOAM[m_registers.oamAddress.data];

                        if(m_scanline > 239){
                            m_registers.oamAddress.data++;
                        }
                        break;

                    // PPU Data.
                    case 0x0007:
                        buffer = m_dataBuffer;
                        m_dataBuffer = ppuBusRead(m_internalRegisters.v.data & 0x7FFF);

                        if((m_internalRegisters.v.data & 0x7FFF) > 0x3F00) buffer = m_dataBuffer;


                        // Visible scanlines + prerender.
                        if( (m_registers.ppumask.bits.showBackground || m_registers.ppumask.bits.showSprites) &&
                            m_scanline >= -1 && m_scanline <= 239
                                ){
                            verticalIncrement();
                            horizontalIncrement();
                        } else {

                            if(m_registers.ppuctrl.bits.incMode)
                                m_internalRegisters.v.data += 32;
                            else
                                m_internalRegisters.v.data++;

                            m_internalRegisters.v.data &= 0x7FFF;
                        }

                    // Other registers are write-only.
                    default:
                        break;
                }

                buffer &= 0xFF;
                return true;
            },

            .write = [&](uint32_t address, uint32_t data){

                // Check whether address is inside the PPU address space.
                // 8 registers are mirrored across the whole space -> ANDing with 0x7.
                if(address >= 0x2000 && address <= 0x3FFF) {
                    address &= 0x7;
                } else {
                    return false;
                }

                switch(address){

                    // Control. Write only.
                    case 0x0000:

                        if(
                                (data & 0x80) &&
                                !m_registers.ppuctrl.bits.nmi &&
                                m_registers.ppustatus.bits.vBlank
                                )
                            m_INT.send();

                        m_registers.ppuctrl.data = data;
                        m_internalRegisters.t.bits.nameX = m_registers.ppuctrl.bits.nameX;
                        m_internalRegisters.t.bits.nameY = m_registers.ppuctrl.bits.nameY;

                        break;

                    // Mask. Write only.
                    case 0x0001:
                        m_registers.ppumask.data = data;
                        break;

                    // OAM address. Write only.
                    case 0x0003:
                        m_registers.oamAddress.data = data;
                        break;

                    /**
                     * OAM data.
                     * Outside of rendering, it will modify OAM at the address specified by 0x2003 and increment the 0x2003.
                     * During rendering (pre-render + visible scanlines), it will not modify OAM but increment the OAMADDR in
                     * a wrong fashion (only the top 6 bits - sprite index).
                    */
                    case 0x0004:
                        if(
                                m_scanline >= -1 && m_scanline <= 239 &&
                                (m_registers.ppumask.bits.showBackground || m_registers.ppumask.bits.showSprites)
                                ){
                            m_registers.oamAddress.bits.index++;
                        } else {
                            m_spriteData.primaryOAM[m_registers.oamAddress.data] = data;
                            m_registers.oamAddress.data++;
                        }
                        break;

                    // Scroll. Write only.
                    case 0x0005:

                        if(!m_internalRegisters.w){

                            m_internalRegisters.t.bits.coarseX = (data & 0xF8) >> 3;
                            m_internalRegisters.x = data & 0x7;
                            m_internalRegisters.w = true;
                        } else {

                            m_internalRegisters.t.bits.fineY = data & 0x7;
                            m_internalRegisters.t.bits.coarseY = (data & 0xF8) >> 3;
                            m_internalRegisters.w = false;
                        }
                        break;

                    // PPU address. Write only.
                    case 0x0006:

                        if(!m_internalRegisters.w){

                            m_internalRegisters.t.data &= 0x40FF; // last bit is cleared as well
                            m_internalRegisters.t.data |= (data & 0x3F) << 8;
                            m_internalRegisters.w = true;
                        } else {

                            m_internalRegisters.t.data &= 0x7F00;
                            m_internalRegisters.t.data |= data;
                            m_internalRegisters.v.data = m_internalRegisters.t.data & 0x7FFF;
                            m_internalRegisters.w = false;
                        }
                        break;

                    // PPU Data.
                    case 0x0007:
                        ppuBusWrite(m_internalRegisters.v.data & 0x7FFF, data);


                        // Visible scanlines + prerender.
                        if( (m_registers.ppumask.bits.showBackground || m_registers.ppumask.bits.showSprites) &&
                            m_scanline >= -1 && m_scanline <= 239
                                ){
                            verticalIncrement();
                            horizontalIncrement();
                        } else {

                            if(m_registers.ppuctrl.bits.incMode)
                                m_internalRegisters.v.data += 32;
                            else
                                m_internalRegisters.v.data++;

                            m_internalRegisters.v.data &= 0x7FFF;
                        }

                    // Other registers are read-only.
                    default:
                        break;
                }

                return true;
            }
    });

    m_connectors["CLK"] = std::make_shared<Connector>(SignalInterface{
        .send = [&](){
            clock();
        }
    });

    m_ports["ppuBus"] = &m_ppuBus;
    m_ports["INT"] = &m_INT;
}

// ===============================================

void R2C02::init(){

    m_clock = 0;
    m_scanline = 0;
    m_frameReady = 0;
    m_dataBuffer = 0;

    m_registers.ppuctrl.data = 0;
    m_registers.ppumask.data = 0;
    m_registers.ppustatus.data = 0;
    m_registers.oamAddress.data = 0;

    m_internalRegisters.t.data = 0;
    m_internalRegisters.v.data = 0;
    m_internalRegisters.x = 0;
    m_internalRegisters.w = 0;

    m_backgroundData.ntByte = 0;
    m_backgroundData.atByte = 0;
    m_backgroundData.tileData = 0;
    m_backgroundData.shiftTileLo = 0;
    m_backgroundData.shiftTileHi = 0;
    m_backgroundData.shiftAttrLo = 0;
    m_backgroundData.shiftAttrHi = 0;

    memset(m_palettes, 0, 32);
    memset(m_palettes, 0, sizeof(m_palettes) / sizeof(m_palettes[0]));
    memset(m_nametables, 0, sizeof(m_nametables) / sizeof(m_nametables[0]));
    memset(m_screen, 0, NESSCREENHEIGHT * NESSCREENWIDTH * sizeof(m_screen[0][0]));
    m_spriteData.clear();
}

void R2C02::verticalIncrement(){

    if(m_registers.ppumask.bits.showSprites || m_registers.ppumask.bits.showBackground){
        if(m_internalRegisters.v.bits.fineY < 7){ // Still in a current tile row, proceed to next line.
            m_internalRegisters.v.bits.fineY++;
        } else {

            m_internalRegisters.v.bits.fineY = 0;

            if(m_internalRegisters.v.bits.coarseY == 29){ // Reached end of the nametable tile area.

                m_internalRegisters.v.bits.coarseY = 0;   // Back to the top.
                m_internalRegisters.v.bits.nameY = !m_internalRegisters.v.bits.nameY; // Switching vertical nametable.
            } else if(m_internalRegisters.v.bits.coarseY == 31){ // Reached the end of the nametable. Only go back to the top, no nametable switching.
                m_internalRegisters.v.bits.coarseY = 0;
            } else {
                m_internalRegisters.v.bits.coarseY++; // Next tile row.
            }
        }
    }
}

void R2C02::horizontalIncrement(){

    if(m_registers.ppumask.bits.showSprites || m_registers.ppumask.bits.showBackground){
        if(m_internalRegisters.v.bits.coarseX == 31){
            m_internalRegisters.v.bits.coarseX = 0;
            m_internalRegisters.v.bits.nameX = !m_internalRegisters.v.bits.nameX;
        } else
            m_internalRegisters.v.bits.coarseX++;
    }
}

void R2C02::verticalTransfer(){

    if(m_registers.ppumask.bits.showSprites || m_registers.ppumask.bits.showBackground){
        m_internalRegisters.v.bits.nameY   = m_internalRegisters.t.bits.nameY;
        m_internalRegisters.v.bits.coarseY = m_internalRegisters.t.bits.coarseY;
        m_internalRegisters.v.bits.fineY   = m_internalRegisters.t.bits.fineY;
    }
}

void R2C02::horizontalTransfer(){

    if(m_registers.ppumask.bits.showSprites || m_registers.ppumask.bits.showBackground){
        m_internalRegisters.v.bits.nameX   = m_internalRegisters.t.bits.nameX;
        m_internalRegisters.v.bits.coarseX = m_internalRegisters.t.bits.coarseX;
    }
}

void R2C02::fetchNT(){

    if(m_registers.ppumask.bits.showBackground)
        m_backgroundData.ntByte = ppuBusRead(0x2000 | (m_internalRegisters.v.data & 0x0FFF));
}

void R2C02::fetchAT(){

    if(m_registers.ppumask.bits.showBackground){

        m_backgroundData.atByte = ppuBusRead(
                0x23C0 | (m_internalRegisters.v.data & 0x0C00) | ((m_internalRegisters.v.data >> 4) & 0x38) | ((m_internalRegisters.v.data >> 2) & 0x07)
        );

        /*
        & 0x2 selects coords 2,3;6,7;10,11...
        these two bites will determine the at byte shift
        */
        if(m_internalRegisters.v.bits.coarseY & 0x2) m_backgroundData.atByte >>= 4;
        if(m_internalRegisters.v.bits.coarseX & 0x2) m_backgroundData.atByte >>= 2;
        m_backgroundData.atByte &= 0x3;
    }
}

void R2C02::fetchTileLo(){

    if(m_registers.ppumask.bits.showBackground){

        m_backgroundData.tileData &= 0xFF00;
        m_backgroundData.tileData |= ppuBusRead(
                (m_registers.ppuctrl.bits.backgroundAddress << 12)
                | (m_backgroundData.ntByte << 4)
                | m_internalRegisters.v.bits.fineY
        );
    }
}

void R2C02::fetchTileHi(){

    if(m_registers.ppumask.bits.showBackground){
        m_backgroundData.tileData &= 0x00FF;
        m_backgroundData.tileData |= ppuBusRead(
                (
                        (m_registers.ppuctrl.bits.backgroundAddress << 12)
                        | (m_backgroundData.ntByte << 4)
                        | m_internalRegisters.v.bits.fineY
                ) + 8
        ) << 8;
    }
}

void R2C02::feedShifters(){

    if(m_registers.ppumask.bits.showBackground){

        m_backgroundData.shiftTileLo &= 0xFF00;
        m_backgroundData.shiftTileLo |= (m_backgroundData.tileData & 0x00FF);
        m_backgroundData.shiftTileHi &= 0xFF00;
        m_backgroundData.shiftTileHi |= (m_backgroundData.tileData & 0xFF00) >> 8;

        m_backgroundData.shiftAttrLo &= 0xFF00;
        m_backgroundData.shiftAttrLo |= (m_backgroundData.atByte & 0x1) ? 0xFF : 0x0;
        m_backgroundData.shiftAttrHi &= 0xFF00;
        m_backgroundData.shiftAttrHi |= (m_backgroundData.atByte & 0x2) ? 0xFF : 0x0;
    }
}

void R2C02::shiftShifters(){

    if(m_registers.ppumask.bits.showBackground){

        m_backgroundData.shiftAttrLo <<= 1;
        m_backgroundData.shiftAttrHi <<= 1;
        m_backgroundData.shiftTileHi <<= 1;
        m_backgroundData.shiftTileLo <<= 1;
    }
}

void R2C02::evaluateSprites(){

    if(m_registers.ppumask.bits.showBackground || m_registers.ppumask.bits.showSprites){

        // Data is written on even cycles.
        // On odd cycles, data is only read, so doing everything on the even cycles.
        if(m_clock % 2 == 0){

            // Are there any sprites in the OAM left to evaluate?
            if(m_registers.oamAddress.bits.index < 64){

                // Is there a space left in the secondary OAM?
                if(m_spriteData.secondarySpriteId < 8){


                    // Belongs the sprite to the current scanline (y position)?
                    if(
                            m_spriteData.primaryOAM[m_registers.oamAddress.data] <= m_scanline &&
                            m_spriteData.primaryOAM[m_registers.oamAddress.data] + (m_registers.ppuctrl.bits.spriteSize == 0 ? 8 : 16) > m_scanline
                            ){

                        // Transfer all 4 bytes to the secondary OAM.
                        for(uint8_t i = 0; i < 4; i++){
                            m_spriteData.secondaryOAM[m_spriteData.secondarySpriteId * 4 + i] = m_spriteData.primaryOAM[m_registers.oamAddress.data];
                            m_registers.oamAddress.data++;
                        }

                        m_spriteData.secondarySpriteId++;
                    } else {
                        m_registers.oamAddress.data += 4;
                    }


                } else {

                    // Evaluate OAM[Id * 4 + Byte] as Y coord
                    if(
                            m_spriteData.primaryOAM[m_registers.oamAddress.data] >= m_scanline &&
                            m_spriteData.primaryOAM[m_registers.oamAddress.data] < m_scanline + (m_registers.ppuctrl.bits.spriteSize == 0 ? 8 : 16)
                            ){

                        m_registers.ppustatus.bits.spriteOverflow = 1;
                        m_registers.oamAddress.bits.index++;
                    } else {

                        m_registers.oamAddress.data++;
                    }
                }

            }
        }
    }
}

void R2C02::fetchSprite(uint8_t offset){

    // Used to switch hi/lo byte of the tile.
    m_spriteData.feedTileAddress += offset;

    uint8_t fineY = m_scanline - m_spriteData.feedY;

    // Vertical flip flag check. Flip on:
    if(m_spriteData.attrLatch[m_spriteData.feedIndex] & 0x80){

        // Only for 8x16 sprites: need to switch tiles if neccessary.
        if(m_registers.ppuctrl.bits.spriteSize){

            if(fineY <= 7){
                m_spriteData.feedTileAddress += 16;
            } else {
                fineY -= 7;
            }
        }

        // Invert Y position.
        fineY = 7 - fineY;

        // No vertical flip:
    } else {

        // 8x16 sprites: switch tiles if fineY > 7.
        if(m_registers.ppuctrl.bits.spriteSize){

            if(fineY > 7){

                m_spriteData.feedTileAddress += 16;
                fineY -= 7;
            }
        }
    }

    // The horizontal flip will be taken care of during render.
    if(offset == 0){
        m_spriteData.shiftLo[m_spriteData.feedIndex] = ppuBusRead(m_spriteData.feedTileAddress + fineY);
    } else {
        m_spriteData.shiftHi[m_spriteData.feedIndex] = ppuBusRead(m_spriteData.feedTileAddress + fineY);
    }
}

/**
 *
 * Note: pre-render scanline (261) is -1.
*/
void R2C02::clock(){

    // Flag reset.
    m_scanlineReady = false;
    m_frameReady = true;

    // =======================================================
    // Pre-render scanline.
    // =======================================================

    if(m_scanline == -1){

        // Should be on clock 1 but test passes when this value is 0.
        // Further investigate.
        if(m_clock == 0){
            m_registers.ppustatus.bits.vBlank = 0;
        }
        if(m_clock == 1){
            m_registers.ppustatus.bits.spriteZeroHit = false;
        }
        else if(m_clock >= 280 && m_clock <= 304)
            verticalTransfer();
        else if(m_clock == 339 && m_oddScan && !m_registers.ppumask.bits.showBackground && !m_registers.ppumask.bits.showSprites){
            m_scanline = 0;
            m_clock = 0;
        }

    }

    // =======================================================
    // Visible scanlines + pre-render scanline.
    // =======================================================
    if(m_scanline >= -1 && m_scanline < 240){


        // *************************************************
        // Clocks 1 - 256
        // *************************************************
        if((m_clock >= 1 && m_clock <= 256)){

            shiftShifters();
            // === Data manipulation ===
            switch((m_clock - 1) % 8){

                case 0:
                    feedShifters();
                    break;

                case 1:
                    fetchNT();
                    break;

                case 3:
                    fetchAT();
                    break;

                case 5:
                    fetchTileLo();
                    break;

                case 7:
                    fetchTileHi();
                    horizontalIncrement();
                    break;

            }

            if(m_clock == 256)
                verticalIncrement();
        } else if (m_clock >= 257 && m_clock <= 320){

            m_registers.oamAddress.data = 0;

            if(m_clock == 257){
                feedShifters();
                horizontalTransfer();
            }

            // *************************************************
            // Clocks 321 - 336 - fetching data for the next scanline
            // *************************************************
        } else if(m_clock >= 321 && m_clock <= 337){

            shiftShifters();
            // === Data manipulation ===
            switch((m_clock - 1) % 8){

                case 0:
                    feedShifters();
                    break;

                case 1:
                    fetchNT();
                    break;

                case 3:
                    fetchAT();
                    break;

                case 5:
                    fetchTileLo();
                    break;

                case 7:
                    fetchTileHi();
                    horizontalIncrement();
                    break;

            }
        } else if(m_clock == 338 || m_clock == 340){
            fetchNT();
        }
    }

    // =======================================================
    // Only visible scanlines
    // =======================================================
    if(m_scanline >= 0 && m_scanline <= 239){

        // Secondary OAM clear occurs 1-64, reading 0x2004 return 0xFF, so cleaning at the first clock.
        if(m_clock == 1){

            // Secondary OAM clear + internal regs preparation.
            m_spriteData.renderInit();
        } else if(m_clock >= 65 && m_clock <= 256){
            evaluateSprites();

        } else if(m_clock >= 257 && m_clock <= 320 && (m_registers.ppumask.bits.showBackground || m_registers.ppumask.bits.showSprites)){

            if(m_clock == 257){
                m_spriteData.shiftClear();
            }

            switch((m_clock - 1) % 8){

                // Fetching an Y coordinate.
                case 0:
                    m_spriteData.feedY = m_spriteData.secondaryOAM[m_spriteData.feedIndex * 4];
                    break;

                    // Fetching a tile ID.
                case 1:

                    // Notice: tile ID needs to be multiplied by 16 (shift by 4),
                    // because in the address it covers bits 11-4.

                    // 8x16 sprites
                    // Patern table is selected by bit 0, bits 7-1 select the tile (= tile ID is multiplied by 2),
                    // the tile is used for the upper part, immediate next part is used for the bottom.
                    if(m_registers.ppuctrl.bits.spriteSize){

                        m_spriteData.feedTileAddress =
                                (((uint16_t)m_spriteData.secondaryOAM[m_spriteData.feedIndex * 4 + 1] & 0xFE) << 4)
                                | (((uint16_t)m_spriteData.secondaryOAM[m_spriteData.feedIndex * 4 + 1] & 0x1) << 12);

                        // 8x8 sprites
                        // Pattern table is selected by the value in PPUCTRL and index is whole value in OAM.
                    } else {
                        m_spriteData.feedTileAddress =
                                ((uint16_t)m_spriteData.secondaryOAM[m_spriteData.feedIndex * 4 + 1] << 4)
                                | ((uint16_t)m_registers.ppuctrl.bits.spriteAddress << 12);
                    }
                    break;

                    // Fetching an attribute.
                case 2:
                    m_spriteData.attrLatch[m_spriteData.feedIndex] = m_spriteData.secondaryOAM[m_spriteData.feedIndex * 4 + 2];
                    break;

                    // Fetching a X coordinate.
                case 3:
                    m_spriteData.x[m_spriteData.feedIndex] = m_spriteData.secondaryOAM[m_spriteData.feedIndex * 4 + 3];
                    break;

                    // Low sprite tile byte.
                case 5:
                    fetchSprite(0);
                    break;
                    // High sprite tile byte.
                case 7:
                    fetchSprite(8);
                    m_spriteData.feedIndex++;
                    break;

            }
        }
    }

    // =======================================================
    // Scanline 241 - Start of the vertical blanking lines.
    // =======================================================
    if(m_scanline == 241 && m_clock == 1){
        m_spriteData.clear(); // OAM decay simulation.

        if(!m_blockNMI){
            m_registers.ppustatus.bits.vBlank = 1;
            if(m_registers.ppuctrl.bits.nmi){
                m_INT.send();
            }
        }
        m_blockNMI = false;
    }

    // NMI multiple trigger simulation.
    // if(
    //     (m_scanline == 241 && m_clock >= 1) ||
    //     (m_scanline >= 242 && m_scanline <= 260)
    //     ){

    //     if(m_registers.ppuctrl.bits.nmi && m_registers.ppustatus.bits.vBlank)
    //         m_nmi = true;
    // }


    // === Pixel placement ===
    // scan <= 239?
    if(m_scanline >= 0 && m_scanline <= 238 && m_clock >= 0 && m_clock <= 255){

        uint8_t bgPixel = 0;
        uint8_t fgPixel = 0;
        uint8_t bgAttr = 0;
        uint8_t fgAttr = 0;
        bool priorityBit = false;


        uint8_t sprIndex = 255;
        uint8_t sprMask = 0;

        // Sprite rendering.
        if(m_registers.ppumask.bits.showSprites){

            // Sprite X position decrement, shifters shift.
            for(uint8_t i = 0; i < 8; i++){

                if(m_spriteData.x[i] > 0)
                    m_spriteData.x[i]--;
                else{

                    // Check if the pixel is non-transparent if no sprite was found yet.
                    if(sprIndex >= 8){
                        sprMask = ((m_spriteData.attrLatch[i] & 0x40) > 0) ? 0x01 : 0x80;
                        if((m_spriteData.shiftLo[i] & sprMask) > 0 || (m_spriteData.shiftHi[i] & sprMask) > 0){
                            sprIndex = i;
                        }
                    }

                    m_spriteData.allowShift[i] = 1;
                }
            }

            // Found non-transparent pixel of a sprite to render.
            if(sprIndex < 8){
                fgPixel
                        = ((m_spriteData.shiftLo[sprIndex] & sprMask) > 0)
                          | (((m_spriteData.shiftHi[sprIndex] & sprMask) > 0) << 1);

                fgAttr = (m_spriteData.attrLatch[sprIndex] & 0x3) + 4;  // +4 = move to the sprite area.
                priorityBit = m_spriteData.attrLatch[sprIndex] & 0x20;
            }

            m_spriteData.shift();

        }

        if(m_registers.ppumask.bits.showBackground){

            uint16_t mask = 0x8000 >> m_internalRegisters.x;
            bgPixel = ((m_backgroundData.shiftTileLo & mask) > 0) | (((m_backgroundData.shiftTileHi & mask) > 0) << 0x1);
            bgAttr = ((m_backgroundData.shiftAttrLo & mask) > 0) | (((m_backgroundData.shiftAttrHi & mask) > 0) << 0x1);
            /*
            m_screen[m_clock][m_scanline]
            = getPixelColor(
                ((m_backgroundData.shiftAttrLo & mask) > 0) | (((m_backgroundData.shiftAttrHi & mask) > 0) << 0x1)
                ,
                ((m_backgroundData.shiftTileLo & mask) > 0) | (((m_backgroundData.shiftTileHi & mask) > 0) << 0x1)
            );
            */
        }

        // Both pixels transparent = render 0x3F00.
        if(fgPixel == 0 && bgPixel == 0)
            m_screen[m_clock][m_scanline] = m_colors[ppuBusRead(0x3F00) & 0x3F];
            // Background transparent, sprite not = render sprite.
        else if(bgPixel == 0 && fgPixel > 0)
            m_screen[m_clock][m_scanline] = getPixelColor(fgAttr, fgPixel);
            // Sprite transparent, background not = render background.
        else if(bgPixel > 0 && fgPixel == 0)
            m_screen[m_clock][m_scanline] = getPixelColor(bgAttr, bgPixel);
        else if(bgPixel > 0 && fgPixel > 0){

            if(
                    sprIndex == 0
                    && m_registers.ppumask.bits.showSprites
                    && m_registers.ppumask.bits.showBackground
                    && m_clock != 255
                    && (m_clock >= 8 || (!m_registers.ppumask.bits.showBackgroundLeft && !m_registers.ppumask.bits.showSpritesLeft))
                    ){
                m_registers.ppustatus.bits.spriteZeroHit = 1;
            }

            // 1 = background priority = render bg.
            if(priorityBit)
                m_screen[m_clock][m_scanline] = getPixelColor(bgAttr, bgPixel);
                // 0 = sprite priority = render sprite.
            else
                m_screen[m_clock][m_scanline] = getPixelColor(fgAttr, fgPixel);
        }
    }

    /* Noise
    if(m_clock < 256 && m_scanline < 240)
        m_screen[m_clock][m_scanline] = m_colors[(rand() % 2 == 0) ? 0x3F : 0x30];
    */

    m_clock++;
    if(m_clock >= 341){

        m_clock = 0;
        m_scanline++;
        m_scanlineReady = true;

        if(m_scanline >= 261){

            m_scanline = -1;
            m_frameReady = true;
        }
    }

    m_oddScan = !m_oddScan;
}

// ===============================================

uint8_t R2C02::ppuBusRead(uint16_t addr){

    uint8_t data = 0x00;
    addr &= 0x3FFF;

    // Palette area mirrored by 32.
    if(addr >= 0x3F00 && addr <= 0x3FFF){

        addr %= 0x20;

        if(m_registers.ppumask.bits.grayscale)
            addr &= 0x30;

        if(addr == 0x10) addr = 0x00;
        else if(addr == 0x14) addr = 0x04;
        else if(addr == 0x18) addr = 0x08;
        else if(addr == 0x1C) addr = 0x0C;

        data = m_palettes[addr];

    // Route other addresses to PPU bus.
    } else {
        data = m_ppuBus.read(addr);
    }

    return data;
}

/**
 * Write to the internal PPU bus.
 * For more info see ppuBusRead.
 * @param addr Address to write to.
 * @param data Data to write.
*/
void R2C02::ppuBusWrite(uint16_t addr, uint8_t data){

    addr &= 0x3FFF;

    // Palette area.
    if(addr >= 0x3F00 && addr <= 0x3FFF){

        addr %= 0x20;

        if(addr == 0x10) addr = 0x00;
        else if(addr == 0x14) addr = 0x04;
        else if(addr == 0x18) addr = 0x08;
        else if(addr == 0x1C) addr = 0x0C;

        m_palettes[addr] = data;

    // Route other addresses to PPU bus.
    } else {
        m_ppuBus.write(addr, data);
    }
}

// ===============================================

void R2C02::OAMDMA(uint8_t addr, uint8_t data){
    m_spriteData.primaryOAM[(addr + m_registers.oamAddress.data) & 0xFF] = data;
}

// ===============================================
R2C02::RGB_t R2C02::getPixelColor(uint8_t paletteId, uint8_t pixel){

    RGB_t color = m_colors[ppuBusRead(0x3F00 + paletteId * 4 + pixel) & 0x3F];

    // Color emphasis.
    if(
            m_registers.ppumask.bits.eRed &&
            m_registers.ppumask.bits.eGreen &&
            m_registers.ppumask.bits.eBlue
            ){
        desaturate(color.blue, 50);
        desaturate(color.green, 50);
        desaturate(color.red, 50);
    }else if(m_registers.ppumask.bits.eBlue){
        saturate(color.blue, 50);
        desaturate(color.green, 50);
        desaturate(color.red, 50);
    } else if(m_registers.ppumask.bits.eGreen){
        desaturate(color.blue, 50);
        saturate(color.green, 50);
        desaturate(color.red, 50);
    } else if(m_registers.ppumask.bits.eRed){
        desaturate(color.blue, 50);
        desaturate(color.green, 50);
        saturate(color.red, 50);
    }

    return color;
}

// http://locklessinc.com/articles/sat_arithmetic/
uint8_t R2C02::saturate(uint8_t x, uint8_t y){

    uint8_t res = x + y;
    res |= -(res < x);

    return res;
}
uint8_t R2C02::desaturate(uint8_t x, uint8_t y){

    uint8_t res = x - y;
    res &= -(res <= x);

    return res;
}

// ===============================================
std::vector<R2C02::RGB_t> R2C02::getPalette(uint8_t paletteId){

    std::vector<RGB_t> palette;

    for(uint8_t i = 0; i < 4; i++){
        palette.push_back(getPixelColor(paletteId, i));
    }

    return palette;
}

uint8_t *R2C02::getPaletteRAM(){

    return m_palettes;
}

std::vector<R2C02::RGB_t> R2C02::getPatternTable(uint8_t paletteId, uint8_t index){

    const int tileSize = 16;
    const int rowSize = 256;
    const int tableSize = 4096;
    std::vector<RGB_t> result;
    result.resize(16384);

    for(uint8_t y = 0; y < 16; y++){

        for(uint8_t x = 0; x < 16; x++){

            int currentOffset = y * rowSize + x * tileSize;
            for(uint8_t tileRow = 0; tileRow < 8; tileRow++){

                uint8_t tileLo = ppuBusRead(index * tableSize + currentOffset + tileRow);
                uint8_t tileHi = ppuBusRead(index * tableSize + currentOffset + tileRow + 8);

                for(uint8_t tileCol = 0; tileCol < 8; tileCol++){

                    uint8_t pixel = ((tileLo & 0x80) >> 7) | ((tileHi & 0x80) >> 6);
                    tileLo <<= 1;
                    tileHi <<= 1;

                    result.at((x * 8 + tileCol) + ((y * 8 + tileRow) * 128)) = getPixelColor(paletteId, pixel);
                }
            }
        }
    }

    return result;
}

std::vector<R2C02::RGB_t> R2C02::getScreen(){

    /*
    for(size_t x = 0; x < 32; x++){

        for(size_t y = 0; y < 30; y++){

            size_t tileId = ppuBusRead(0x2000 + x + y * 32) * 16;

            for(uint8_t tileRow = 0; tileRow < 8; tileRow++){

                uint8_t tileLo = ppuBusRead(tileId + tileRow);
                uint8_t tileHi = ppuBusRead(tileId + tileRow + 8);

                for(uint8_t tileCol = 0; tileCol < 8; tileCol++){

                    uint8_t pixel = ((tileLo & 0x80) >> 7) | ((tileHi & 0x80) >> 6);
                    tileLo <<= 1;
                    tileHi <<= 1;

                    m_screen[x * 8 + tileCol][y * 8 + tileRow] = getPixelColor(0, pixel);
                }
            }
        }
    }
    */

    std::vector<RGB_t> screen;
    screen.resize(NESSCREENHEIGHT * NESSCREENWIDTH);

    for(uint16_t y = 0; y < NESSCREENHEIGHT; y++){

        for(uint16_t x = 0; x < NESSCREENWIDTH; x++){

            screen.at(x + y * NESSCREENWIDTH) = m_screen[x][y];
        }
    }

    return screen;
}

bool R2C02::scanlineFinished() const{
    return m_scanlineReady;
}

bool R2C02::frameFinished() const{
    return m_frameReady;
}

std::vector<EmulatorWindow> R2C02::getGUIs() {


    std::function<void(void)> screen = [this](){

        static float scale = 1.0f;

        // Window contents
        // ===================================================================
        ImGui::SliderFloat("Scale", &scale, 1.0, 10.0);
        ImDrawList * dl = ImGui::GetWindowDrawList();
        const ImVec2 defaultScreenPos = ImGui::GetCursorScreenPos();

        for(size_t imageX = 0; imageX < NESSCREENHEIGHT; imageX++) {
            for(size_t imageY = 0; imageY < NESSCREENWIDTH; imageY++) {

                ImColor color = ImColor(
                        m_screen[imageY][imageX].red,
                        m_screen[imageY][imageX].green,
                        m_screen[imageY][imageX].blue,
                        255
                );

                float screenX = defaultScreenPos.x + scale * static_cast<float>(imageX);
                float screenY = defaultScreenPos.y + scale * static_cast<float>(imageY);
                dl->AddRectFilled({screenX, screenY}, {screenX + scale, screenY + scale}, color);
            }
        }

        float dummyWidth = scale * static_cast<float>(NESSCREENWIDTH);
        float dummyHeight = scale * static_cast<float>(NESSCREENHEIGHT);
        // Dummy widget is needed for scrollbars to work and to allow to place more elements below correctly.
        ImGui::Dummy({dummyWidth, dummyHeight});
    };

    return {
            EmulatorWindow{
                    .category = m_deviceName,
                    .title = " Screen",
                    .id    = getDeviceID(),
                    .dock  = DockSpace::MAIN,
                    .guiFunction = screen
            }
    };
}
