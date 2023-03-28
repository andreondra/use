#include "systems/NES.h"

NES::NES() {

    // Connect CPU to the main system bus as master.
    m_cpu.connect("mainBus", m_cpuBus.getConnector("master"));

    // Connect slave components to the main system bus (CPU bus).
    m_cpuBus.connect("slot 0", m_RAM.getConnector("data"));
    m_cpuBus.connect("slot 1", m_ppu.getConnector("cpuBus"));
    m_cpuBus.connect("slot 2", m_cart.getConnector("cpuBus"));

    // Connect PPU as master to the PPU bus.
    m_ppu.connect("ppuBus", m_ppuBus.getConnector("master"));
    // Connect slave components to the PPU bus.
    m_ppuBus.connect("slot 0", m_cart.getConnector("ppuBus"));

    // Connect PPU's INT pin to 6502's NMI.
    m_ppu.connect("INT", m_cpu.getConnector("NMI"));

    // Connect CPU's and PPU's clock to the system clock.
    m_cpuClock.connect(m_cpu.getConnector("CLK"));
    m_ppuClock.connect(m_ppu.getConnector("CLK"));

    // Load components to make base class "aware" of the components
    // to show GUI, correctly initialize the system etc.
    m_components.push_back(&m_cpuBus);
    m_components.push_back(&m_cpu);
    m_components.push_back(&m_RAM);
    m_components.push_back(&m_ppu);
    m_components.push_back(&m_cart);
};

void NES::clock() {

    m_ppuClock.send();
    if(m_clockCount % 3 == 0){

        m_cpuClock.send();
        if(m_clockCount % 2 == 0){
            // APU clock.
        }
    }

    m_clockCount++;
}

void NES::doClocks(unsigned int count) {
    for(int i = 0; i < count; i++)
        clock();
}

void NES::doSteps(unsigned int count) {
    while(!m_cpu.instrFinished())
        clock();
    clock();
}

void NES::doFrames(unsigned int count) {

}

void NES::doRun(unsigned int updateFrequency) {

    if(updateFrequency > PPU_CLOCK_HZ)
        throw std::invalid_argument("Update frequency too high!");

    // Calculate how many clocks to run based on function call interval.
    unsigned int remainingClocks = PPU_CLOCK_HZ / updateFrequency;

    while(remainingClocks) {
        clock();
        remainingClocks--;
    }
}

