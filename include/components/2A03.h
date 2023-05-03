/**
 * @file 2A03.cpp
 * @author Ondrej Golasowski (golasowski.o@gmail.com)
 * @brief Ricoh 2A03 (MOS 6502 variant) emulation.
 * @copyright Copyright (c) 2023 Ondrej Golasowski
 */

#ifndef USE_2A03_H
#define USE_2A03_H

#include "components/6502.h"

/**
 * NES CPU emulation class. Because the 2A03 is a clone of the 6502, the RP2A03 inherits from the 6502
 * and modifies the behaviour slightly. It contains all the ports and connectors of the 6502 and adds the OAMDMA.
 *
 * Additional connectors: data "OAMDMA", which is used to trigger the DMA unit.
 *
 * @note Normally the 2A03 contains also the APU but that is separated for a sake of modularity in this project.
 * */
class RP2A03 : public MOS6502 {

protected:

public:
    RP2A03();
    ~RP2A03() override;

    void init() override;
};

#endif //USE_2A03_H
