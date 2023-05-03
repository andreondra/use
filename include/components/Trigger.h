/**
 * @file Trigger.h
 * @author Ondrej Golasowski (golasowski.o@gmail.com)
 * @brief Data to signal connector converter.
 * @copyright Copyright (c) 2023 Ondrej Golasowski
 */

#ifndef USE_TRIGGER_H
#define USE_TRIGGER_H

#include "Component.h"

/**
 * This component converts data connector to signal one: when a value is written to a specified address,
 * it will be masked and checked for equivalency. If the value equals to the one specified a signal will be sent
 * to the connected component.
 *
 * Connectors: data "trigger" - writing to this connector will trigger a "target" connected device if addresses match.
 * Ports: signal "target" - the signal is sent to the component connected to this port on write to the "trigger"
 *
 * @note It can be used to map signals to memory, this way a CPU can trigger an interrupt signal by
 * writing to a specified address.
 * */
class Trigger : public Component {
protected:
    uint32_t m_triggerAddress, m_triggerValue, m_mask;
    SignalPort m_target;

public:
    explicit Trigger(uint32_t address,
                     uint32_t value = 0x1,
                     uint32_t mask = ~0x0);

    void init() override;
    std::vector<EmulatorWindow> getGUIs() override;
};

#endif //USE_TRIGGER_H
