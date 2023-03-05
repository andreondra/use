//
// Created by golas on 23.2.23.
//

#include "components/Bus.h"

Bus::Bus(int portCount, int addrWidth, int dataWidth) : m_devices(portCount) {

    if(addrWidth < 0 || addrWidth > 32)
        throw std::invalid_argument("Address width not in range [0,32].");

    m_addrMask = (~(uint32_t)0x0) >> (32 - addrWidth);

    if(dataWidth < 0 || dataWidth > 32)
        throw std::invalid_argument("Data width not in range [0,32].");

    m_dataMask = (~(uint32_t)0x0) >> (32 - dataWidth);

    Connector masterConnector(
        DataInterface{
            .read = [&](uint32_t address, uint32_t & buffer) {

                for(auto & device : m_devices)
                    if(device.readConfirmed(address & m_addrMask, buffer)) {
                        buffer &= m_dataMask;
                        return true;
                    }

                return false;
            },
            .write = [&](uint32_t address, uint32_t data) {

                for(auto & device : m_devices)
                    device.write(address & m_addrMask, data & m_dataMask);
            }
        }
    );

    m_connectors["master"] = std::make_shared<Connector>(masterConnector);

    for(size_t i = 0; i < m_devices.size(); i++)
        m_ports["slot " + std::to_string(i)] = &m_devices[i];

}

void Bus::init() {

}

std::vector<std::function<void(void)>> Bus::getGUIs() {
    return std::vector<std::function<void(void)>>();
}