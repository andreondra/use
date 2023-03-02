//
// Created by golas on 2.3.23.
//

#include "components/Memory.h"

template<uint32_t size>
Memory<size>::Memory(
    AddressRange addressRange,
    uint8_t defaultValue)

    : m_addressRange(addressRange),
      m_defaultValue(defaultValue) {

    m_dataConnector.setInterface(
        DataInterface{
             .read = [&](uint32_t address) {

                 if(m_addressRange.has(address))
                     return m_data[(address % size) + m_addressRange.from];
             },

             .write = [&](uint32_t address, uint32_t data){
                 if(m_addressRange.has(address))
                     m_data[(address % size) + m_addressRange.from] = data;
            }
     });

    m_connectors["data"] = &m_dataConnector;
}

template<uint32_t size>
void Memory<size>::init() {
    m_data.fill(m_defaultValue);
}

template<uint32_t size>
void Memory<size>::renderGUI() {

}

