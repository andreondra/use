//
// Created by golas on 21.2.23.
//

#ifndef USE_SYSTEM_H
#define USE_SYSTEM_H

class System{

private:

public:
    System() = default;

    virtual void init()     = 0;
    virtual void reset()    = 0;

    virtual void doClocks(int count)        = 0;
    virtual void doSteps(int count)         = 0;
    virtual void doFrames(int count)        = 0;
    virtual void doRun(int updateFrequency) = 0;
};

#endif //USE_SYSTEM_H
