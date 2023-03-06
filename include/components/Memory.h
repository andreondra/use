//
// Created by golas on 2.3.23.
//

#ifndef USE_MEMORY_H
#define USE_MEMORY_H

#include <cstdint>
#include <memory>
#include "Component.h"
#include "Types.h"

class Memory : public Component{

private:
    std::vector<uint8_t> m_data;
    AddressRange m_addressRange;
    uint8_t m_defaultValue;

    void memoryInit();

public:
    Memory(
        size_t size,
        AddressRange addressRange,
        uint8_t defaultValue = 0x0);

    void init() override;
    std::vector<std::function<void(void)>> getGUIs() override;

    void load(uint32_t from, std::ifstream & src);
};

#endif //USE_MEMORY_H
