//
// Created by golas on 22.2.23.
//

#include "systems/Bare6502.h"

Bare6502::Bare6502() {

    m_systemName = "Bare 6502";

    m_bus.connect("slot 1", m_RAM.getConnector("data"));
    m_cpu.connect("mainBus", m_bus.getConnector("master"));

    m_components.push_back(&m_bus);
    m_components.push_back(&m_RAM);
    m_components.push_back(&m_cpu);
}

void Bare6502::init() {

}

void Bare6502::doClocks(unsigned int count) {

}

void Bare6502::doSteps(unsigned int count) {

}

void Bare6502::doFrames(unsigned int count) {

}

void Bare6502::doRun(unsigned int updateFrequency) {

}
