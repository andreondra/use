//
// Created by golas on 23.2.23.
//

#ifndef USE_BUS_H
#define USE_BUS_H

#include <map>
#include <cstdint>
#include <stdexcept>
#include <cstdint>
#include "Types.h"
#include "Component.h"

/**
 * A simple bus abstraction class with a primitive arbitration mechanism.
 * */
class Bus : public Component{

private:
    /// Address lane width.
    int m_addrWidth;
    /// Data lane width.
    int m_dataWidth;

//    /**
//     * Read data from devices on the bus. There's a simple arbitration based on a device priority.
//     * If multiple devices respond to the address and have a same priority, the selection method is not defined.
//     * However, usually only one device respond to a particular address.
//     *
//     * @param address Address to read from.
//     * @return Data read.
//     * */
//    uint32_t busRead(uint32_t address);
//
//    /**
//     * Write data to all devices on the bus. It is up to the device to process or ignore the data written.
//     * @param address Address to write to.
//     * @param data Data to write.
//     * */
//    void busWrite(uint32_t address, uint32_t data);

public:
    Bus(int addrWidth, int dataWidth);
};

#endif //USE_BUS_H
