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

public:
    Bus(int portCount, int addrWidth, int dataWidth);

    void init() override;
    std::vector<std::function<void(void)>> getGUIs() override;
};

#endif //USE_BUS_H