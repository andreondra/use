//
// Created by golas on 23.2.23.
//

#include "imgui.h"
#include "components/Bus.h"

Bus::Bus(int portCount, int addrWidth, int dataWidth) : m_devices(portCount) {

    if(addrWidth < 0 || addrWidth > 32)
        throw std::invalid_argument("Address width not in range [0,32].");

    m_addrMask = (~(uint32_t)0x0) >> (32 - addrWidth);

    if(dataWidth < 0 || dataWidth > 32)
        throw std::invalid_argument("Data width not in range [0,32].");

    m_dataMask = (~(uint32_t)0x0) >> (32 - dataWidth);

    m_deviceName = "Bus";

    Connector masterConnector(
        DataInterface{
            .read = [&](uint32_t address, uint32_t & buffer) {

                for(auto & device : m_devices)
                    if(device.readConfirmed(address & m_addrMask, buffer)) {
                        buffer &= m_dataMask;

                        m_lastAccess = lastAccess::READ;
                        m_lastAddress = address & m_addrMask;
                        m_lastData = buffer;
                        return true;
                    }

                return false;
            },
            .write = [&](uint32_t address, uint32_t data) {

                for(auto & device : m_devices)
                    device.write(address & m_addrMask, data & m_dataMask);

                m_lastAccess = lastAccess::WRITE;
                m_lastAddress = address & m_addrMask;
                m_lastData = data & m_dataMask;
            }
        }
    );

    m_connectors["master"] = std::make_shared<Connector>(masterConnector);

    for(size_t i = 0; i < m_devices.size(); i++)
        m_ports["slot " + std::to_string(i)] = &m_devices[i];

}

void Bus::init() {

}

std::vector<EmulatorWindow> Bus::getGUIs() {

    std::function<void(void)> debugger = [this](){

        // Window contents
        // ===================================================================
        ImGui::Text("Address mask: 0x%x", m_addrMask);
        ImGui::Text("Data mask: 0x%x", m_dataMask);
        ImGui::Text("Connected devices: %lu", m_devices.size());
        ImGui::Separator();
        ImGui::Text("Last access");
        if(m_lastAccess != lastAccess::NONE) {

            ImGui::Text("Type: ");
            ImGui::SameLine();
            switch (m_lastAccess) {
                case lastAccess::READ: ImGui::Text("read"); break;
                case lastAccess::WRITE: ImGui::Text("write"); break;
                default: ImGui::Text("unknown"); break;
            }

            ImGui::Text("At address: 0x%x", m_lastAddress);
            ImGui::Text("Data: 0x%x", m_lastData);
        } else {
            ImGui::Text("There was no operation on the bus.");
        }

    };

    return {
            EmulatorWindow{
                    .category = m_deviceName,
                    .title = "Debugger",
                    .id    = getDeviceID(),
                    .dock  = DockSpace::LEFT,
                    .guiFunction = debugger
            }
    };
}