/**
 * @file Types.h
 * Emulator helper types.
 * */

#ifndef USE_TYPES_H
#define USE_TYPES_H

#include <cstdint>
#include <functional>

struct DataInterface {

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
};

struct SignalInterface {
    std::function<void(void)> send;
};

struct AddressRange {

    uint32_t from;
    uint32_t to;

    [[nodiscard]] bool has(const uint32_t & value) const {
        return (from <= value) && (value <= to);
    }
};

#endif //USE_TYPES_H
