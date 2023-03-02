//
// Created by golas on 2.3.23.
//

#ifndef USE_MEMORY_H
#define USE_MEMORY_H

#include <cstdint>
#include "Component.h"
#include "Types.h"

template<uint32_t size>
class Memory : public Component{

private:
    std::array<uint8_t, size> m_data;
    Connector m_dataConnector;
    AddressRange m_addressRange;
    uint8_t m_defaultValue;

public:
    explicit Memory(
        AddressRange addressRange,
        uint8_t defaultValue = 0x0);

    void init() override;
    void renderGUI() override;
};

#endif //USE_MEMORY_H
