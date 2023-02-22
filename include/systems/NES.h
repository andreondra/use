//
// Created by golas on 21.2.23.
//

#ifndef USE_NES_H
#define USE_NES_H

#include "System.h"

class NES : public System{
private:

public:
    NES() = default;

    void init() override;
    void reset() override;

    void doClocks(int count) override;
    void doSteps(int count) override;
    void doFrames(int count) override;
    void doRun(int updateFrequency) override;

};

#endif //USE_NES_H
