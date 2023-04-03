//
// Created by golas on 17.3.23.
//

#include "System.h"

System::System() {

    // Get audio output of all components.

    for(auto & component : m_components)
        for(auto & source : component->getSoundSampleSources())
            m_sampleSources.push_back(source);
}

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

SoundStereoFrame System::getAudioFrame(size_t outputIndex) const {

    if(outputIndex > m_sampleSources.size())
        throw std::invalid_argument("Invalid audio output index!");

    return m_sampleSources[outputIndex]();
}

