//
// Created by golas on 22.2.23.
//

#ifndef USE_BARE6502_H
#define USE_BARE6502_H

#include "System.h"
#include "components/6502.h"

class Bare6502 : public System{
private:
    MOS6502 cpu;
    
public:
    Bare6502() = default;

    void init() override;
    void reset() override;

    void doClocks(int count) override;
    void doSteps(int count) override;
    void doFrames(int count) override;
    void doRun(int updateFrequency) override;

};

#endif //USE_BARE6502_H
