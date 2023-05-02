/**
 * @file NES.h
 * @author Ondrej Golasowski (golasowski.o@gmail.com)
 * @brief Nintendo Entertainment System SW emulator.
 * @copyright Copyright (c) 2023 Ondrej Golasowski
 */

#ifndef USE_NES_H
#define USE_NES_H

#include "System.h"
#include "components/2A03.h"
#include "components/APU.h"
#include "components/2C02.h"
#include "components/Bus.h"
#include "components/Memory.h"
#include "components/Gamepak/Gamepak.h"
#include "components/NESPeripherals.h"

/**
 * Nintendo Entertainment System emulation (NTSC version).
 * */
class NES : public System{
protected:
    // ===========================================
    // Constants
    // ===========================================
    const unsigned int MASTER_CLOCK_HZ = 21477272;
    const unsigned int PPU_CLOCK_HZ = MASTER_CLOCK_HZ / 4;

    // ===========================================
    // System components
    // ===========================================
    RP2A03 m_cpu;
    APU m_apu;
    R2C02 m_ppu;
    Memory m_RAM{0x800, {.from = 0x0000, .to = 0x1FFF}, 0xFF};
    Gamepak m_cart;
    Bus m_cpuBus{5, 16, 8};
    Bus m_ppuBus{1, 14, 8};
    NESPeripherals m_peripherals;
    std::shared_ptr<Connector> m_apuPeripheralConnector;

    SignalPort m_cpuClock, m_ppuClock, m_apuClock;

    // ===========================================
    // Emulation helper data
    // ===========================================
    unsigned long long m_clockCount = 0;

    // ===========================================
    // Emulation helper functions
    // ===========================================
    void clock();
public:
    NES();
    ~NES() override = default;

    void doClocks(unsigned int count) override;
    void doSteps(unsigned int count) override;
    void doFrames(unsigned int count) override;
    void doRun(unsigned int updateFrequency) override;
};

#endif //USE_NES_H
