//
// Created by golas on 22.2.23.
//

#include "systems/Bare6502.h"

Bare6502::Bare6502() {

    m_bus.connect("slot 1", m_RAM.getConnector("data"));
    m_cpu.connect("mainBus", m_bus.getConnector("master"));
}

Bare6502::~Bare6502() {

}

void Bare6502::init() {

}

void Bare6502::doClocks(int count) {

}

void Bare6502::doSteps(int count) {

}

void Bare6502::doFrames(int count) {

}

void Bare6502::doRun(int updateFrequency) {

}
