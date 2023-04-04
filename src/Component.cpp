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

    std::vector<std::string> keys;
    for(auto & con : m_connectors) {
        keys.push_back(con.first);
    }
    return keys;
}


std::vector<std::string> Component::listPorts() const {

    std::vector<std::string> keys;
    for(auto & port : m_ports) {
        keys.push_back(port.first);
    }
    return keys;
}