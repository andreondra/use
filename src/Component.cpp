//
// Created by golas on 21.2.23.
//

#include <stdexcept>
#include <utility>
#include <ranges>
#include "Component.h"

uintptr_t Component::getDeviceID() const {
    return reinterpret_cast<uintptr_t>(this);
}

std::string Component::getDeviceName() const {
    return m_deviceName;
}

void Component::setDeviceName(const std::string &newName) {
    m_deviceName = newName;
}

void Component::connect(const std::string &toPort, std::weak_ptr<Connector> connector) {

    if(!m_ports.contains(toPort)) {
        throw std::invalid_argument("Specified port not exists.");
    }

    m_ports[toPort]->connect(std::move(connector));
}

void Component::disconnect(const std::string &fromPort) {

    if(!m_ports.contains(fromPort)) {
        throw std::invalid_argument("Specified port not exists.");
    }

    m_ports[fromPort]->disconnect();
}


std::weak_ptr<Connector> Component::getConnector(const std::string & name) {

    if(!m_connectors.contains(name)) {
        throw std::invalid_argument("Specified connector not exists.");
    }

    return m_connectors[name];
}

std::vector<std::string> Component::listConnectors() const {

    auto names = std::views::keys(m_connectors);
    return {names.begin(), names.end()};
}


std::vector<std::string> Component::listPorts() const {

    auto names = std::views::keys(m_ports);
    return {names.begin(), names.end()};
}

bool Component::initRequested() {

    bool prevValue = m_initRequested;
    m_initRequested = false;

    return prevValue;
}