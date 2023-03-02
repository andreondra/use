//
// Created by golas on 1.3.23.
//

#include "Connector.h"

Connector::Connector(SignalInterface interface) {
    m_interface.emplace<SignalInterface>(std::move(interface));
}

Connector::Connector(DataInterface interface) {
    m_interface.emplace<DataInterface>(std::move(interface));
}

bool Connector::hasDataInterface() const {
    return std::holds_alternative<DataInterface>(m_interface);
}

bool Connector::hasSignalInterface() const {
    return std::holds_alternative<SignalInterface>(m_interface);
}

const DataInterface & Connector::getDataInterface() {
    return std::get<DataInterface>(m_interface);
}

const SignalInterface & Connector::getSignalInterface() {
    return std::get<SignalInterface>(m_interface);
}

void Connector::setInterface(DataInterface interface) {
    m_interface = std::move(interface);
}

void Connector::setInterface(SignalInterface interface) {
    m_interface = std::move(interface);
}

