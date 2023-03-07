//
// Created by golas on 6.3.23.
//

#include "components/Trigger.h"

Trigger::Trigger(uint32_t address, uint32_t value) : m_triggerAddress(address), m_triggerValue(value) {

    Connector triggerConnector(
            DataInterface{
                    .read = [&](uint32_t address, uint32_t & buffer) {
                        return false;
                    },

                    .write = [&](uint32_t address, uint32_t data) {
                        if(address == m_triggerAddress && data == m_triggerValue)
                            m_target.send();
                    }
            });

    m_connectors["trigger"] = std::make_shared<Connector>(triggerConnector);
    m_ports["target"] = &m_target;
}

void Trigger::init() { }

std::vector<std::function<void(void)>> Trigger::getGUIs() {
    return std::vector<std::function<void(void)>>();
}
