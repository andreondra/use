#include "components/APU.h"
#include <cstdlib>
#include <cstdio>
#include <cmath>
#include "Tools.h"
#include <cassert>

APU::APU() : m_pulse1(false), m_pulse2(true) {

    m_deviceName = "APU";

    m_connectors["CLK"] = std::make_shared<Connector>(SignalInterface{
            .send = [this](){
                clock();
            }
    });

    m_connectors["cpuBus"] = std::make_shared<Connector>(DataInterface{

            .read = [&](uint32_t address, uint32_t & buffer) -> bool {

                if(address == 0x4015){
                        buffer =
                                (m_pulse1.lengthCounter.counterValue > 0) |
                                ((m_pulse2.lengthCounter.counterValue > 0) << 1) |
                                //((m_noise.lengthCounter.value > 0) << 3) |
                                (m_internalIRQState << 6);

                        m_IRQ.set(false);
                        m_internalIRQState = false;

                        return true;
                } else {
                    return false;
                }
            },

            .write = [&](uint32_t address, uint32_t data){

                switch(address){

                    // Pulse 1 envelope configuration and duty bits.
                    case 0x4000:
                        m_pulse1.envelope.configure(data);
                        m_pulse1.dutyCycle = (data & 0xC0) >> 6;
                        m_pulse1.lengthCounter.setHaltFlag(data & 0x20);
                        break;

                        // Pulse 1 sweep setup.
                    case 0x4001:
                        m_pulse1.setupSweep(data);
                        break;

                        // Pulse 1 timer low.
                    case 0x4002:
                        m_pulse1.timerPeriod &= 0x700;
                        m_pulse1.timerPeriod |= data;
                        m_pulse1.updateTargetPeriod();
                        break;

                    case 0x4003:
                        m_pulse1.envelope.setStart(true);
                        m_pulse1.sequencerPos = 0;
                        m_pulse1.timerPeriod &= 0xFF;
                        m_pulse1.timerPeriod |= (data & 0x7) << 8;
                        m_pulse1.timer = m_pulse1.timerPeriod;
                        m_pulse1.lengthCounter.setLength((data & 0xF8) >> 3);
                        m_pulse1.updateTargetPeriod();
                        break;

                        // Pulse 2 envelope configuration and duty bits.
                    case 0x4004:
                        m_pulse2.envelope.configure(data);
                        m_pulse2.dutyCycle = (data & 0xC0) >> 6;
                        m_pulse2.lengthCounter.setHaltFlag(data & 0x20);
                        break;

                        // Pulse 2 sweep setup.
                    case 0x4005:
                        m_pulse2.setupSweep(data);
                        break;

                        // Pulse 2 timer low.
                    case 0x4006:
                        m_pulse2.timerPeriod &= 0x700;
                        m_pulse2.timerPeriod |= data;
                        m_pulse2.updateTargetPeriod();
                        break;

                    case 0x4007:
                        m_pulse2.envelope.setStart(true);
                        m_pulse2.sequencerPos = 0;
                        m_pulse2.timerPeriod &= 0xFF;
                        m_pulse2.timerPeriod |= (data & 0x7) << 8;
                        m_pulse2.timer = m_pulse2.timerPeriod;
                        m_pulse2.lengthCounter.setLength((data & 0xF8) >> 3);
                        m_pulse2.updateTargetPeriod();
                        break;

                    case 0x400B:
                        m_triangle.lengthCounter.setLength((data & 0xF8) >> 3);
                        break;

                        // Noise envelope configuration and duty bits.
                    case 0x400C:
                        m_noise.envelope.configure(data);
                        break;

                    case 0x400E:
                        m_noise.modeFlag = (data & 0x80);
                        m_noise.setPeriod(data & 0xF);
                        break;

                    case 0x400F:
                        m_noise.lengthCounter.setLength((data & 0xF8) >> 3);
                        m_noise.envelope.setStart(true);
                        break;

                    case 0x4015:

                        m_pulse1.lengthCounter.setEnableFlag(data & 0x1);
                        m_pulse2.lengthCounter.setEnableFlag(data & 0x2);
                        m_noise.lengthCounter.setEnableFlag(data & 0x8);
                        break;

                    case 0x4017:
                        frameCounterModeFlag = data & 0x80;
                        disableFrameInterruptFlag = data & 0x40;
                        if(disableFrameInterruptFlag) {
                            m_IRQ.set(false);
                            m_internalIRQState = false;
                        }
                        break;

                    default:
                        break;
                }
            }
    });

    m_ports["IRQ"] = &m_IRQ;
}

void APU::init(){

    m_clock = 0;
    frameCounterModeFlag = false;
    disableFrameInterruptFlag = false;

    m_pulse1.reset();
    m_pulse2.reset();
    m_noise.reset();
    m_triangle.reset();
}

void APU::clock(){

    if(m_clock == 3728){ // Actually should be 3728.5.

        m_pulse1.envelope.clock();
        m_pulse2.envelope.clock();
        m_noise.envelope.clock();

    } else if(m_clock == 7456){ // Actually should be 7456.5.

        m_pulse1.envelope.clock();
        m_pulse2.envelope.clock();
        m_noise.envelope.clock();

        m_pulse1.lengthCounter.clock();
        m_pulse2.lengthCounter.clock();
        m_noise.lengthCounter.clock();

        m_pulse1.clockSweep();
        m_pulse2.clockSweep();

    } else if(m_clock == 11185){ // Actually should be 11185.5.

        m_pulse1.envelope.clock();
        m_pulse2.envelope.clock();
        m_noise.envelope.clock();

    } else if(m_clock == 14914){

        m_pulse1.envelope.clock();
        m_pulse2.envelope.clock();
        m_noise.envelope.clock();

        m_pulse1.lengthCounter.clock();
        m_pulse2.lengthCounter.clock();
        m_noise.lengthCounter.clock();

        m_pulse1.clockSweep();
        m_pulse2.clockSweep();

        if(!disableFrameInterruptFlag) {
            m_IRQ.set(true);
            m_internalIRQState = true;
        }

    } else if(m_clock == 14915){

        if(!disableFrameInterruptFlag) {
            m_IRQ.set(true);
            m_internalIRQState = true;
        }

    } else if(m_clock == 18640 && frameCounterModeFlag){

        m_pulse1.envelope.clock();
        m_pulse2.envelope.clock();
        m_noise.envelope.clock();
    }

    m_pulse1.clock();
    m_pulse2.clock();
    m_noise.clock();

    uint16_t maxClock;
    if(frameCounterModeFlag){
        maxClock = 18641;
    } else {
        maxClock = 14915;
    }

    if(m_clock >= maxClock)
        m_clock = 0;
    else
        m_clock++;
}

double APU::output(){


    double pulse;
    if(m_pulse1.output() + m_pulse2.output()  == 0)
        pulse = 0;
    else
        pulse = 95.88 / ((8128.0 / (m_pulse1.output() + m_pulse2.output())) + 100);

    double tnd = 159.79 / ( (1.0 / (m_noise.output() / 12241.0)) + 100 );

    return pulse + tnd;
}

float APU::oscOutput(){


    int16_t noise = 0;//USETools::map(m_noise.output(), 0, 15, -1, 1);
    return m_pulse1.oscOutput() / 3 + m_pulse2.oscOutput() / 3 + noise / 8;
}

std::vector<EmulatorWindow> APU::getGUIs() {
    return {};
}

SoundSampleSources APU::getSoundSampleSources() {

    return {
      [&](){

          float sample =  output();
          SoundStereoFrame frame{sample, sample};
          return frame;
      }
    };
}

void APU::apu_lengthCounter::reset(){

    haltFlag = false;
    enableFlag = false;
    counterValue = 0;
}

void APU::apu_lengthCounter::clock(){

    if(counterValue > 0 && !haltFlag)
        counterValue--;
}

void APU::apu_lengthCounter::setLength(uint8_t lengthBits){

    if(enableFlag){
        counterValue = lengths[lengthBits & 0x1F];
    }
}

void APU::apu_lengthCounter::setEnableFlag(bool value){

    enableFlag = value;

    if(!enableFlag){
        counterValue = 0;
    }
}

void APU::apu_lengthCounter::setHaltFlag(bool value){
    haltFlag = value;
}

void APU::apu_envelope::reset(){

    decayLevelCounter = 0;
    divider = 0;
    startFlag = false;
    loopFlag = false;
    constantVolumeFlag = false;
    dividerPeriodReloadValue = 0;
}

void APU::apu_envelope::setStart(bool value){
    startFlag = value;
}

void APU::apu_envelope::configure(uint8_t configuration){

    loopFlag = configuration & 0x20;
    constantVolumeFlag = configuration & 0x10;
    dividerPeriodReloadValue = configuration & 0xF;
}

void APU::apu_envelope::clock(){

    if(startFlag){
        startFlag = false;
        decayLevelCounter = 15;
        divider = dividerPeriodReloadValue;

        // Divider clocking.
    } else {

        if(divider == 0){
            divider = dividerPeriodReloadValue;

            // Decay level counter clocking.
            if(decayLevelCounter == 0){

                if(loopFlag){
                    decayLevelCounter = 15;
                }
            } else {
                decayLevelCounter--;
            }
        } else {
            divider--;
        }
    }
}

uint8_t APU::apu_envelope::output(){

    if(constantVolumeFlag)
        return dividerPeriodReloadValue;
    else
        return decayLevelCounter;
}

APU::apu_pulse::apu_pulse(bool twosComplement) : useTwosComplement(twosComplement){}

void APU::apu_pulse::reset(){

    sequencerPos = 0;
    timer = 0;
    timerPeriod = 0;
    dutyCycle = 0;
    envelope.reset();
    lengthCounter.reset();
    sweepReload = false;
    sweepEnabled = false;
    sweepNegate = false;
    sweepShiftCount = 0;
    sweepPeriod = 0;
    sweepCounter = 0;
    targetPeriod = 0;
    phaseIndex = 0;
}

void APU::apu_pulse::setupSweep(uint8_t value){

    sweepEnabled = value & 0x80;
    sweepPeriod = value & 0x70 >> 4;
    sweepNegate = value & 0x8;
    sweepShiftCount = value & 0x7;
    sweepReload = true;
}

void APU::apu_pulse::clockSweep(){

    if(sweepCounter == 0 && sweepEnabled && timerPeriod >= 8 && targetPeriod <= 0x7FF){

        updateTargetPeriod();
        timerPeriod = targetPeriod;
    }

    if(sweepCounter == 0 || sweepReload){
        sweepCounter = sweepPeriod;
        sweepReload = false;
    } else {
        sweepCounter--;
    }
}

void APU::apu_pulse::updateTargetPeriod(){

    if(sweepShiftCount > 0){

        if(sweepNegate){

            if(useTwosComplement)
                targetPeriod = timerPeriod - (timerPeriod >> sweepShiftCount);
            else
                targetPeriod = timerPeriod - (timerPeriod >> sweepShiftCount) - 1;
        } else {
            targetPeriod = timerPeriod + (timerPeriod >> sweepShiftCount);
        }
    }
}

void APU::apu_pulse::clock(){

    // Timer clocking.
    if(timer == 0){
        timer = timerPeriod;

        // Sequencer clocking.
        if(sequencerPos >= 7)
            sequencerPos = 0;
        else
            sequencerPos++;
    } else {
        timer--;
    }

    // Oscilator phase index. Emulator helper variable.
    if(phaseIndex >= 1000000)
        phaseIndex -= 1000000;
    else
        phaseIndex += (1789773 / (16 * (timerPeriod + 1))) / 894886.5;
}

uint8_t APU::apu_pulse::output(){

    if(
            ((sequences[dutyCycle] << sequencerPos) & 0x80) == 0 ||
            targetPeriod > 0x7FF ||
            lengthCounter.counterValue == 0 ||
            timer < 8
            )
        return 0;
    else{
        return envelope.output();
    }
}

float APU::apu_pulse::oscOutput(){

    if(
            targetPeriod > 0x7FF ||
            lengthCounter.counterValue == 0 ||
            timerPeriod < 8
            )
        return 0;
    else{

        // Mapping envelope output from [0, 15] to [0, max amplitude] range.
        float amplitude = (envelope.output() / 7.5f) - 1;
        float sum = 0;
        for(int i = 1; i < 25; i++){
            sum += (1.0f/(float)i)*sin(M_PI*i*sequencesOsc[dutyCycle])*cos(i*2*M_PI*phaseIndex);
        }

        float sample = /*amplitude * sequencesOsc[dutyCycle]*/ + (2*amplitude)/(M_PI) * sum;
        //outputValue += ((int16_t)(amplitude * sin(2.0f * M_PI * f * time)) - outputValue) / 2; // sine
        return sample; // pulse
    }
}

void APU::apu_noise::reset(){

    periodIndex = 0;
    timer = 0;
    shiftRegister = 0;
    envelope.reset();
    lengthCounter.reset();
    modeFlag = false;
}

void APU::apu_noise::setPeriod(uint8_t bits){

    periodIndex = bits & 0xF; // Max index is 15.
    timer = periods[periodIndex];
}

void APU::apu_noise::clock(){

    if(timer == 0){
        timer = periods[periodIndex];

        // Shift register clock.
        uint8_t feedback = (shiftRegister & 0x1) ^ (( shiftRegister >> (1 + (modeFlag * 5)) ) & 0x1);
        shiftRegister >>= 1;
        shiftRegister &= 0x3FFF;
        shiftRegister |= (uint16_t)feedback << 14;
    } else {
        timer--;
    }
}

uint8_t APU::apu_noise::output(){

    if(lengthCounter.counterValue == 0 || shiftRegister & 0x1)
        return 0;
    else
        return envelope.output();
}

void APU::apu_triangle::reset(){

    timerPeriod = 0;
    timer = 0;
    linearCounter = 0;
    envelope.reset();
    lengthCounter.reset();
}
