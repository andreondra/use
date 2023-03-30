//
// Created by golas on 21.2.23.
//

#ifndef USE_SYSTEM_H
#define USE_SYSTEM_H

#include <vector>
#include "Types.h"
#include "Component.h"

class System {

protected:
    std::vector<Component *> m_components;

public:
    System() = default;
    virtual ~System() = default;

    virtual void init();

    virtual void doClocks(unsigned int count)        = 0;
    virtual void doSteps(unsigned  int count)         = 0;
    virtual void doFrames(unsigned int count)        = 0;
    virtual void doRun(unsigned int updateFrequency) = 0;

    /**
     * Get all the GUI elements created by the System and underlying Component(s).
     *
     * @return Vector of dockable windows.
     * */
    virtual std::vector<EmulatorWindow> getGUIs();
};

#endif //USE_SYSTEM_H
