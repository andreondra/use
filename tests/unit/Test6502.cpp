//
// Created by golas on 5.3.23.
//

#include <fstream>
#include "gtest/gtest.h"
#include "components/6502.h"

TEST(Test6502, Functional) {

    const uint32_t MEM_SIZE = 65536;
    // This is an address from a corresponding .lst file where the 'success' trap
    // is located.
    const uint16_t ADR_SUCCESS = 0x3699;

    // Create a mock memory device and load a test ROM.
    std::vector<uint8_t> memory(MEM_SIZE);
    std::ifstream testROM("testfiles/6502_functional_test.bin", std::ios_base::binary);
    ASSERT_TRUE(testROM) << "Can't open test ROM.";
    testROM.read((char*)memory.data(), memory.size());
    ASSERT_TRUE(testROM) << "Can't load test ROM.";

    // Create a connector for the mock memory.
    std::shared_ptr<Connector> memCon = std::make_shared<Connector>(
        DataInterface{
            .read = [&memory](uint32_t address, uint32_t & buffer) -> bool{
                buffer = memory.at(address);
                return true;
            },
            .write = [&memory](uint32_t address, uint32_t data){
                memory.at(address) = data;
            }
        }
    );

    // Create a CPU and connect the memory.
    // No other devices will be connected, so the bus can be represented by the memory.
    class DUT : public MOS6502 {
    public:
        void step() {
            while(!instrFinished()) {
                CLK();
            }
            CLK();
        }
        uint16_t getPC() {
            return m_registers.pc;
        }
        void setPC(uint16_t val) {
            m_registers.pc = val;
        }
    };

    DUT cpu;
    cpu.connect("mainBus", memCon);
    cpu.setPC(0x400);

    uint16_t prevPC;

    do{
        prevPC = cpu.getPC();
        cpu.step();
    } while(prevPC != cpu.getPC());

    EXPECT_EQ(prevPC, ADR_SUCCESS) << "The test failed on trap at address 0x" << std::hex << prevPC;
}

TEST(Test6502, Interrupt) {

    const uint32_t MEM_SIZE = 65536;
    // This is an address from a corresponding .lst file where the 'success' trap
    // is located.
    const uint16_t ADR_SUCCESS = 0x06e5;
    const uint16_t FEEDBACK_REG = 0xbffc;
    const uint16_t IRQ_MASK = 0x1;
    const uint16_t NMI_MASK = 0x2;

    // Create a mock memory device and load a test ROM.
    std::vector<uint8_t> memory(MEM_SIZE);
    std::ifstream testROM("testfiles/6502_interrupt_test.bin", std::ios_base::binary);
    ASSERT_TRUE(testROM) << "Can't open test ROM.";
    testROM.read((char*)memory.data(), memory.size());
    ASSERT_TRUE(testROM) << "Can't load test ROM.";

    // Create a CPU and connect the memory.
    // No other devices will be connected, so the bus can be represented by the memory.
    class DUT : public MOS6502 {
    public:
        void step() {
            while(!instrFinished()) {
                CLK();
            }
            CLK();
        }
        uint16_t getPC() {
            return m_registers.pc;
        }
        void setPC(uint16_t val) {
            m_registers.pc = val;
        }
        void triggerNMI() {
            NMI();
        }
        void setIRQ(bool active) {
            IRQ(active);
        }
    };

    DUT cpu;
    uint16_t prevPC;

    // Create a connector for the mock memory.
    std::shared_ptr<Connector> memCon = std::make_shared<Connector>(
            DataInterface{
                    .read = [&memory](uint32_t address, uint32_t & buffer) -> bool{
                        buffer = memory.at(address);
                        return true;
                    },
                    .write = [&memory, &cpu, FEEDBACK_REG, NMI_MASK, IRQ_MASK](uint32_t address, uint32_t data){
                        memory.at(address) = data;
                        if(address == FEEDBACK_REG) {
                            if(!(data & NMI_MASK)) cpu.triggerNMI();
                            cpu.setIRQ(!(data & IRQ_MASK));
                        }
                    }
            }
    );

    cpu.connect("mainBus", memCon);
    cpu.setPC(0x400);

    do{
        prevPC = cpu.getPC();
        cpu.step();
    } while(prevPC != cpu.getPC());

    EXPECT_EQ(prevPC, ADR_SUCCESS) << "The test failed on trap at address 0x" << std::hex << prevPC;
}