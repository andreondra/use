//
// Created by golas on 2.3.23.
//

#include "components/Memory.h"
#include <fstream>

Memory::Memory(
    size_t size,
    AddressRange addressRange,
    uint8_t defaultValue)

    : m_data(size),
      m_addressRange(addressRange),
      m_defaultValue(defaultValue) {

    Connector dataConnector(
        DataInterface{
             .read = [&](uint32_t address, uint32_t & buffer) {

                 if(m_addressRange.has(address)) {
                     buffer = m_data[(address - m_addressRange.from) % m_data.size()];
                     return true;
                 } else {
                     return false;
                 }

             },

             .write = [&](uint32_t address, uint32_t data){
                 if(m_addressRange.has(address))
                     m_data[(address - m_addressRange.from) % m_data.size()] = data;
             }
     });

    m_connectors["data"] = std::make_shared<Connector>(dataConnector);

    memoryInit();
}

void Memory::memoryInit() {
    std::fill(m_data.begin(), m_data.end(), m_defaultValue);
}

void Memory::init() {
    memoryInit();
}

std::vector<std::function<void(void)>> Memory::getGUIs() {
    return std::vector<std::function<void(void)>>();
}

void Memory::load(uint32_t startOffset, std::ifstream & src) {

    if(startOffset > m_data.size()) {
        throw std::invalid_argument("Offset is bigger than the memory size.");
    }

    src.read((char *)(m_data.data() + startOffset), m_data.size() - startOffset);
}

