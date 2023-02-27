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

std::vector<std::string> Component::listDataPorts() const {

    auto names = std::views::keys(m_dataPorts);
    return {names.begin(), names.end()};
}


void Component::connectData(const std::string &portName, DataInterface connector) {

    if(m_dataPorts.find(portName) == m_dataPorts.end()) {
        throw std::invalid_argument("Specified interface not available.");
    }

    m_dataPorts[portName] = std::move(connector);
}

void Component::disconnectData(const std::string &portName) {

    if(m_dataPorts.find(portName) == m_dataPorts.end()) {
        throw std::invalid_argument("Specified interface not available.");
    }

    m_dataPorts[portName].beDummy();
}

std::vector<std::string> Component::listDataConnectors() const {

    auto names = std::views::keys(m_dataConnectors);
    return {names.begin(), names.end()};
}

DataInterface Component::getDataConnector(const std::string &connectorName) {

    if(m_dataConnectors.find(connectorName) == m_dataConnectors.end()) {
        throw std::invalid_argument("Specified interface not available.");
    }

    return m_dataConnectors[connectorName];
}

std::vector<std::string> Component::listPinPorts() const {

    auto names = std::views::keys(m_pinPorts);
    return {names.begin(), names.end()}
}

void Component::connectPin(const std::string &portName, std::function<void(void)> connector) {

    if(m_pinPorts.find(portName) == m_pinPorts.end()) {
        throw std::invalid_argument("Specified interface not available.");
    }

    m_pinPorts[portName] = std::move(connector);
}

void Component::disconnectPin(const std::string &portName) {

    if(m_dataPorts.find(portName) == m_dataPorts.end()) {
        throw std::invalid_argument("Specified interface not available.");
    }

    m_pinPorts[portName] = [](){};
}

std::vector<std::string> Component::listPinConnectors() const {

    auto names = std::views::keys(m_pinConnectors);
    return {names.begin(), names.end()}
}

std::function<void(void)> Component::getPinConnector(const std::string &pinName) {

    if(m_dataConnectors.find(pinName) == m_dataConnectors.end()) {
        throw std::invalid_argument("Specified interface not available.");
    }

    return m_pinConnectors[pinName];
}
