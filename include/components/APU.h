/**
 * @file APU.h
 * @author Ondrej Golasowski (golasowski.o@gmail.com)
 * @brief NES Audio Processing Unit emulation.
 * @copyright Copyright (c) 2023 Ondrej Golasowski
 */

#ifndef USE_APU_H
#define USE_APU_H

#include <cstdint>
#include "Component.h"

class APU : public Component {

private:

    std::vector<float> m_samples;

    bool m_internalIRQState = false;
    /**
     * Main APU clock.
    */
    uint16_t m_clock = 0;

    // Toggles between 4 and 5 step FC sequences.
    bool frameCounterModeFlag = false;

    // Disables IRQ.
    bool disableFrameInterruptFlag = false;

    // Decremented on every APU cycle, except when == 0 or halted.
    struct apu_lengthCounter{

    private:
        uint8_t lengths[0x20] = {
                10, 254, 20, 2, 40, 4, 80, 6, 160, 8, 60, 10, 14, 12, 26, 14,
                12, 16, 24, 18, 48, 20, 96, 22, 192, 24, 72, 26, 16, 28, 32, 30
        };
        bool haltFlag = false;
        bool enableFlag = false;

    public:
        uint8_t counterValue = 0;
        void reset();
        void clock();
        void setLength(uint8_t lengthBits);
        void setEnableFlag(bool value);
        void setHaltFlag(bool value);
    };

    /**
     * APU Envelope.
     * Generates a volume pulse that is then modified by sound channels.
    */
    struct apu_envelope{

    private:

        // A value of the output during fade-out.
        uint8_t decayLevelCounter = 0;

        // Fade-out contdown timer.
        uint8_t divider = 0;
        bool startFlag = false;
        bool loopFlag = false;
        bool constantVolumeFlag = false;

    public:

        // Divider restore value and also a constant volume value.
        uint8_t dividerPeriodReloadValue = 0;

        /**
         * Reset envelope to the initial state.
        */
        void reset();

        /**
         * Configure the envelope with the 0x4000/0x4004/0x400c byte.
         * @param configuration Conf byte with the following structure:
         * xxLC NNNN
         * x = not used
         * L = loop flag
         * C = constant volume flag
         * N = envelope period/volume
        */
        void configure(uint8_t configuration);

        /**
         * Set the start flag.
         * @param value Value of the flag.
        */
        void setStart(bool value);

        /**
         * Proceed one clock in the emulation.
        */
        void clock();

        /**
         * Envelope output.
         * @return Volume value (0-15).
        */
        uint8_t output();
    };

    // ================================================================================================
    // APU sound channels
    // Modify volume envelope to create a unique sound.

    // ====================== APU Pulse ======================
    //                  Sweep -----> Timer
    //                    |            |
    //                    |            |
    //                    |            v
    //                    |        Sequencer   Length Counter
    //                    |            |             |
    //                    |            |             |
    //                    v            v             v
    // Envelope -------> Gate -----> Gate -------> Gate --->(to mixer)
    // Envelope generates the signal, then it is passed through 3 gates.
    // First gate...
    // Second gate is controlled by sequencer, which opens/closes the gate
    // according to the selected sequence (sequence controls the duty cycle of the signal),
    // which is shifted by the timer (timer controls the frequency of the signal).
    // Third gate is controlled by the length counter.
    // Finally, the signal is passed to the mixer.
    struct apu_pulse{

        apu_pulse(bool useTwosComplement);

        /**
         * Duty Cycle Sequences
         * This array contains sequencer output waveforms:
         * 0: 0100 0000 (12.5 %)
         * 1: 0110 0000 (25 %)
         * 2: 0111 1000 (50 %)
         * 3: 1001 1111 (25 % negated)
        */
        const uint8_t sequences[4] = {0x40, 0x60, 0x78, 0x9F};
        const double sequencesOsc[4] = {0.125, 0.25, 0.5, 0.75};

        /**
         * Current position in the sequence (value 0-7).
         * The position is updated when the timer goes from 0 to t (underflows).
        */
        uint8_t sequencerPos = 0;

        // 11-bit timer current value.
        uint16_t timer = 0;
        // Timer reset value.
        uint16_t timerPeriod = 0;

        // Current duty cycle (sequences array 2-bit index).
        uint8_t dutyCycle = 0;

        // Volume envelope.
        apu_envelope envelope;

        // Length counter.
        apu_lengthCounter lengthCounter;

        // Sweep unit.
        bool useTwosComplement = false;
        bool sweepReload = false;
        bool sweepEnabled = false;
        bool sweepNegate = false;
        uint8_t sweepShiftCount = 0;
        uint8_t sweepPeriod = 0;
        uint8_t sweepCounter = 0;
        uint16_t targetPeriod = 0;

        void setupSweep(uint8_t value);
        void clockSweep();
        void updateTargetPeriod();

        // Oscillator index in the generated pulse.
        double phaseIndex = 0;

        void reset();
        void clock();
        uint8_t output();
        float oscOutput();
    } m_pulse1, m_pulse2;

    struct apu_noise{

    private:
        uint8_t periodIndex = 0;
        const uint16_t periods[0x10] = {
                4, 8, 16, 32, 64, 96, 128, 160, 202, 254, 380, 508, 762, 1016, 2034, 4068
        };
        uint16_t timer = 0;
        uint16_t shiftRegister = 1;

    public:

        apu_envelope envelope;
        apu_lengthCounter lengthCounter;
        bool modeFlag = false;

        /**
         * Reset the noise channel to its initial state.
        */
        void reset();
        void setPeriod(uint8_t bits);
        void clock();
        uint8_t output();
    } m_noise;

    struct apu_triangle{

    private:


    public:
        uint16_t timerPeriod;
        uint16_t timer;
        uint8_t linearCounter;

        apu_envelope envelope;
        apu_lengthCounter lengthCounter;

        void reset();
    }m_triangle;

public:

    // Outbound interrupt request flag.
    SignalPort m_IRQ;

    APU();
    ~APU() override = default;

    /**
     * Reset APU to the initial state.
    */
    void init() override;

    /**
     * APU master clock. Emulates the APU Frame Counter (Sequencer).
     * APU is clocked on every other CPU cycle (2 CPU cycles == 1 APU cycle).
    */
    void clock();

    /**
     * Raw audio output. Not usable.
     * @return Approx audio level from 0.0 to 1.0.
    */
    double output();

    /**
     * Continuous output generated by an oscillator.
     * Clear audio output that can be sent directly to the sound driver.
     * @return Audio level from -32768 to 32767 (16 bit int range).
    */
    float oscOutput();

    std::vector<EmulatorWindow> getGUIs() override;

    SoundSampleSources  getSoundSampleSources() override;
};

#endif //USE_APU_H
