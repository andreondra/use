/**
 * @file Memory.h
 * @author Ondrej Golasowski (golasowski.o@gmail.com)
 * @brief Universal memory.
 * @copyright Copyright (c) 2023 Ondrej Golasowski
 */

#ifndef USE_MEMORY_H
#define USE_MEMORY_H

#include <cstdint>
#include <memory>
#include "Component.h"
#include "Types.h"

/**
 * @brief This component is an universal software model of a rewritable memory.
 *
 * The memory can have a size specified, a range to which it will be mapped to and a default value.
 * Ports: none
 * Connectors: data connector "data" to access the memory.
 *
 * @note Please note that the address range can be larger than the size, then the memory will be mirrored
 * across the whole range.
 * */
class Memory : public Component{

private:
    std::vector<uint8_t> m_data;
    AddressRange m_addressRange;
    uint8_t m_defaultValue;

    void memoryInit();

public:
    /**
     * Construct a memory.
     *
     * @param size Size of the memory in bytes.
     * @param addressRange Range to which the memory to.
     * @param defaultValue Default value of the memory on init.
     * */
    Memory(
        size_t size,
        AddressRange addressRange,
        uint8_t defaultValue = 0x0);

    void init() override;
    std::vector<EmulatorWindow> getGUIs() override;

    void load(uint32_t from, std::ifstream & src);
};

#endif //USE_MEMORY_H
