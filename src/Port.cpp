//
// Created by golas on 1.3.23.
//

#include "Port.h"

bool Port::empty() const {
    return m_connector.expired();
}

Port::operator bool() const {
    return empty();
}

void Port::disconnect() {
    m_connector.reset();
}

// =============================================================================

void DataPort::connect(std::weak_ptr<Connector> connector) {

    if(connector.expired())
        throw std::invalid_argument("Provided connector is empty.");
    if(!connector.lock()->hasDataInterface())
        throw std::invalid_argument("Provided connector doesn't have a data interface.");

    m_connector.swap(connector);
}

uint32_t DataPort::read(uint32_t address) {

    if(empty()) {
        return false;
    } else {
        uint32_t buffer;
        if(m_connector.lock()->getDataInterface().read(address, buffer)) {
            return buffer;
        } else {
            return 0;
        }
    }
}

bool DataPort::readConfirmed(uint32_t address, uint32_t &buffer) {

    if(empty())
        return false;
    else
        return m_connector.lock()->getDataInterface().read(address, buffer);
}

void DataPort::write(uint32_t address, uint32_t data) {

    if(!empty())
        return m_connector.lock()->getDataInterface().write(address, data);
}

// =============================================================================

void SignalPort::connect(std::weak_ptr<Connector> connector) {

    if(connector.expired())
        throw std::invalid_argument("Provided connector is empty.");
    if(!connector.lock()->hasSignalInterface())
        throw std::invalid_argument("Provided connector doesn't have a signal interface.");

    m_connector.swap(connector);
}

void SignalPort::send() {

    if(!empty())
        m_connector.lock()->getSignalInterface().send();
}
