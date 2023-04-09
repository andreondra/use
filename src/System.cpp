//
// Created by golas on 17.3.23.
//

#include "System.h"

System::System() { }

void System::init() {
    for(auto & component : m_components)
        component->init();
}

std::vector<EmulatorWindow> System::getGUIs() {

    std::vector<EmulatorWindow> mergedGUIs;

    for(auto & component : m_components)
        for(auto & gui : component->getGUIs())
            mergedGUIs.push_back(gui);

    return mergedGUIs;
}

void System::onRefresh() {

    for(auto & component : m_components) {
        if(component->initRequested()) {
            init();
            break;
        }
    }
}

unsigned long System::getClockRate() const {
    return m_systemClockRate;
}

size_t System::soundOutputCount() const {
    return m_sampleSources.size();
}

const SoundSampleSources & System::getSampleSources() const {
    return m_sampleSources;
}

std::vector<ImInputBinder::action_t> System::getInputs() {

    std::vector<ImInputBinder::action_t> mergedInputs;

    for(auto & component : m_components)
        for(auto & input : component->getInputs())
            mergedInputs.push_back(input);

    return mergedInputs;
}

std::string System::getName() const {
    return m_systemName;
}
