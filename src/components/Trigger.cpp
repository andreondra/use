//
// Created by golas on 6.3.23.
//

#include "components/Trigger.h"

Trigger::Trigger(uint32_t address, uint32_t value, uint32_t mask)
    : m_triggerAddress(address), m_triggerValue(value), m_mask(mask) {

    Connector triggerConnector(
            DataInterface{
                    .read = [&](uint32_t address, uint32_t & buffer) {
                        return false;
                    },

                    .write = [&](uint32_t address, uint32_t data) {
                        if(address == m_triggerAddress && (data & m_mask) == m_triggerValue)
                            m_target.send();
                    }
            });

    m_connectors["trigger"] = std::make_shared<Connector>(triggerConnector);
    m_ports["target"] = &m_target;
}

void Trigger::init() { }

std::vector<EmulatorWindow> Trigger::getGUIs() {

    // This Component doesn't render any window.
    return {};
}
