//
// Created by golas on 6.3.23.
//

#ifndef USE_TRIGGER_H
#define USE_TRIGGER_H

#include "Component.h"

class Trigger : public Component {
protected:
    uint32_t m_triggerAddress, m_triggerValue;
    SignalPort m_target;

public:
    Trigger(uint32_t address,
            uint32_t value = 0x1);

    void init() override;
    std::vector<std::function<void(void)>> getGUIs() override;
};

#endif //USE_TRIGGER_H
