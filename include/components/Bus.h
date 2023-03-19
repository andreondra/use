//
// Created by golas on 23.2.23.
//

#ifndef USE_BUS_H
#define USE_BUS_H

#include <map>
#include <cstdint>
#include <stdexcept>
#include <cstdint>
#include <memory>
#include "Types.h"
#include "Component.h"

/**
 * @brief A simple bus abstraction class with a primitive arbitration mechanism.
 *
 * Available ports: "slot x" where x is in range [0, portCount].
 *
 * Available connectors: "master" to access all devices on the bus.
 * */
class Bus : public Component{

private:
    /// Address lane mask.
    uint32_t m_addrMask;
    /// Data lane mask.
    uint32_t m_dataMask;
    /// Data connections to the devices on the bus.
    std::vector<DataPort> m_devices;

    /// Last access type.
    enum class lastAccess {NONE, READ, WRITE} m_lastAccess = lastAccess::NONE;
    /// Last address accessed (debug info).
    uint32_t m_lastAddress = 0x00;
    /// Last data read/written.
    uint32_t m_lastData = 0x00;

public:
    Bus(int portCount, int addrWidth, int dataWidth);

    void init() override;
    std::vector<EmulatorWindow> getGUIs() override;
};

#endif //USE_BUS_H