//
// Created by golas on 17.3.23.
//

#include "System.h"

std::vector<EmulatorWindow> System::getGUIs() {

    std::vector<EmulatorWindow> mergedGUIs;

    for(auto & component : m_components)
        for(auto & gui : component->getGUIs())
            mergedGUIs.push_back(gui);

    return mergedGUIs;
}