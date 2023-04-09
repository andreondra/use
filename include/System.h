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
    unsigned long m_systemClockRate = 0;
    SoundSampleSources m_sampleSources;

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

    /**
     * Get main clock rate.
     *
     * @return Clock rate.
     * */
    [[nodiscard]] virtual unsigned long getClockRate() const;

    /**
     * Get a count of sound outputs. Used to correctly construct nodes in Sound.h.
     *
     * @return Count of all sound outputs.
     * */
    [[nodiscard]] virtual size_t soundOutputCount() const;

    /**
     * Get sample sources.
     *
     * @return Audio sample getters.
     * */
    [[nodiscard]] virtual const SoundSampleSources & getSampleSources() const;

    /**
     * Get inputs of all components in the System.
     *
     * @return Required inputs.
     * */
    virtual std::vector<ImInputBinder::action_t> getInputs();
};

#endif //USE_SYSTEM_H
