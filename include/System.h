//
// Created by golas on 21.2.23.
//

#ifndef USE_SYSTEM_H
#define USE_SYSTEM_H

#include <vector>
#include "Types.h"
#include "Component.h"
#include "Sound.h"

class System {

protected:
    std::vector<Component *> m_components;
    unsigned long m_systemClockRate = 0;
    SoundSampleSources m_sampleSources;
    Sound *m_soundOutput;

public:
    System();
    virtual ~System() = default;

    virtual void init();

    /**
     * Proceed specified number of system clocks.
     * @param count Number of clock to process.
     * */
    virtual void doClocks(unsigned int count)           = 0;

    /**
     * Proceed specified number of CPU instructions.
     * @param count Number of instructions to process.
     * */
    virtual void doSteps(unsigned  int count)           = 0;

    /**
     * Proceed specified number of rendered frames.
     * @param count Number of frames to process.
     * */
    virtual void doFrames(unsigned int count)           = 0;

    /**
     * Run system in real-time.
     * @param updateFrequency Frequency in which the function is called to match the clock speed.
     * */
    virtual void doRun(unsigned int updateFrequency)    = 0;

    /**
     * Callback that is called on every new frame.
     * */
    virtual void onRefresh();

    /**
     * Get all the GUI elements created by the System and underlying Component(s).
     *
     * @return Vector of dockable windows.
     * */
    virtual std::vector<EmulatorWindow> getGUIs();

    [[nodiscard]] virtual unsigned long getClockRate() const;

    [[nodiscard]] virtual size_t soundOutputCount() const;

    [[nodiscard]] virtual SoundStereoFrame getAudioFrame(size_t index) const;
};

#endif //USE_SYSTEM_H
