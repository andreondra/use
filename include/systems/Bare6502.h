//
// Created by golas on 22.2.23.
//

#ifndef USE_BARE6502_H
#define USE_BARE6502_H

#include "System.h"
#include "components/6502.h"
#include "components/Memory.h"
#include "components/Bus.h"

class Bare6502 : public System{
protected:
    MOS6502 m_cpu;
    Memory m_RAM{0x800, {0x0000, 0x1FFF}, 0xFF};
    Bus m_bus{2, 8, 16};

public:
    Bare6502();
    ~Bare6502() override = default;

    void init() override;

    void doClocks(unsigned int count) override;
    void doSteps(unsigned int count) override;
    void doFrames(unsigned int count) override;
    void doRun(unsigned int updateFrequency) override;

//    std::vector<EmulatorWindow> getGUIs() override;
};

#endif //USE_BARE6502_H
