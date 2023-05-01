/**
 * @file System.h
 * @author Ondrej Golasowski (golasowski.o@gmail.com)
 * @brief System emulation abstraction.
 * @copyright Copyright (c) 2023 Ondrej Golasowski
 *
 */

#ifndef USE_SYSTEM_H
#define USE_SYSTEM_H

#include <vector>
#include "Types.h"
#include "Component.h"
#include "ImInputBinder.h"

/**
 * This class is an abstraction of an emulated system.
 * Components are placed and interconnected here to create a functioning system.
 * */
class System {

protected:
    std::string m_systemName = "Default System";
    std::vector<Component *> m_components;
    unsigned long m_systemClockRate = 0;
    SoundSampleSources m_sampleSources;

public:
    System();
    virtual ~System() = default;

    /**
     * Initialize a System to a power-on state. Default implementation just calls init on all components in m_components.
     *
     * @note For System developer: If there is any other operation except initializing all the components,
     * you can override the method and implement the initialization yourself.
     * */
    virtual void init();

    /**
     * Proceed specified number of system clocks.
     *
     * @note For System developer: implement a reaction to a main clock signal. Normally whole emulation
     * is driven by this function.
     *
     * @param count Number of clock to process.
     * */
    virtual void doClocks(unsigned int count)           = 0;

    /**
     * Proceed specified number of CPU instructions.
     *
     * @note For System developer: You can for example proceed by single clocks until the CPU processes
     * specified number of instructions. If there is no CPU, provide dummy method.
     *
     * @param count Number of instructions to process.
     * */
    virtual void doSteps(unsigned  int count)           = 0;

    /**
     * Proceed specified number of rendered frames.
     *
     * @note For System developer: If there is neither GPU or any definition of frame, provide dummy method.
     *
     * @param count Number of frames to process.
     * */
    virtual void doFrames(unsigned int count)           = 0;

    /**
     * Run system in real-time.
     *
     * @note For System developer:
     *
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

    /// Get system's name.
    [[nodiscard]] virtual std::string getName() const final;
};

#endif //USE_SYSTEM_H
