/**
 * @file Types.h
 * Emulator helper types.
 * */

#ifndef USE_TYPES_H
#define USE_TYPES_H

#include <cstdint>
#include <functional>

/**
 * @brief An interface to access a device from the bus.
 * */
struct DeviceAccess {

    /**
     * Read data from the device.
     * @param address Address to read from.
     * @param[out] target Variable to save the data to.
     * @return 0 Nothing was read (device not responded to the address).
     * @return 1-255 Device responded with the priority equal to the returned value.
     * */
    std::function<uint8_t(uint32_t address, uint32_t & target)> read;

    /**
     * Write data to the device.
     * @param address Address to write to.
     * @param data Data to store.
     * */
    std::function<void(uint32_t address, uint32_t data)> write;
};

struct DataInterface {

    /**
     * Construct a data interface. Empty interfaces are made dummy to be safe to call when not defined.
     * */
    DataInterface() { beDummy(); }

    /**
     * Read data from the device.
     * @param address Address to read from.
     * @return Data read from the address.
     * */
    std::function<uint32_t(uint32_t address)> read;

    /**
     * Write data to the device.
     * @param address Address to write to.
     * @param data Data to store.
     * */
    std::function<void(uint32_t address, uint32_t data)> write;

    /**
     * Change the interface to a dummy (disconnected state).
     * */
    void beDummy() {
        read  = [](uint32_t address){ return 0; };
        write = [](uint32_t address,uint32_t data){ };

    }
};

/**
 * An interface to access a data bus from the device.
 * */
struct BusAccess {

    /**
     * Read data from the bus.
     * @param address Address to read from.
     * @return Data read from the address.
     * */
    std::function<uint32_t(uint32_t address)> read;

    /**
     * Write data to the bus.
     * @param address Address to write to.
     * @param data Data to store.
     * */
    std::function<void(uint32_t address, uint32_t data)> write;
};

#endif //USE_TYPES_H
