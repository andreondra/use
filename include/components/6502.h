/**
 * @file 6502.h
 * @author Ondrej Golasowski (golasowski.o@gmail.com)
 * @brief MOS6502 CPU software implementation.
 * @copyright Copyright (c) 2022 Ondrej Golasowski
 *
 */

#ifndef USE_6502_H
#define USE_6502_H

#include <cstdint>
#include <string>
#include <functional>
#include <map>
#include "Component.h"
#include "Types.h"

class MOS6502 : public Component{
public:
    // ===========================================
    // CPU internals
    // ===========================================
    /// 6502 default stack position.
    static constexpr uint16_t STACK_POSITION = 0x0100;
    /// NMI vector position.
    static constexpr uint16_t VECTOR_NMI = 0xFFFA;
    /// Reset vector position.
    static constexpr uint16_t VECTOR_RST = 0xFFFC;
    /// IRQ vector position.
    static constexpr uint16_t VECTOR_IRQ = 0xFFFE;


    /// A single instruction.
    typedef struct {
        char mnemonic[4]; // ASM mnemonic.
        uint8_t (MOS6502::*addrMode)(); // Pointer to the address mode function.
        uint8_t (MOS6502::*instrCode)(); // Pointer to the instruction code.
        uint8_t instrLen; // Instruction length in bytes.
        uint8_t cycles; // Machine cycles count.
    } instruction_t;

    using m = MOS6502;
    /**
     * Opcode lookup table.
     * Not defined opcodes are assigned mnemonic "???" and execute NOP instruction.
    */
    const instruction_t lookup[256] =
            {  //0                                1                                2                                3                                4                                5                                6                                7                                8                                9                                 A                               B                                C                                D                                E                                F
                    /*0*/{"BRK", &m::IMP, &m::BRK, 1, 7}, {"ORA", &m::IDX, &m::ORA, 2, 6}, {"JAM", &m::IMP, &m::JAM, 1, 1}, {"SLO", &m::IDX, &m::SLO, 2, 8}, {"NOP", &m::ZP0, &m::NOP, 2, 3}, {"ORA", &m::ZP0, &m::ORA, 2, 3}, {"ASL", &m::ZP0, &m::ASL, 2, 5}, {"SLO", &m::ZP0, &m::SLO, 2, 5}, {"PHP", &m::IMP, &m::PHP, 1, 3}, {"ORA", &m::IMM, &m::ORA, 2, 2}, {"ASL", &m::ACC, &m::ASL, 1, 2}, {"ANC", &m::IMM, &m::ANC, 2, 2}, {"NOP", &m::ABS, &m::NOP, 3, 4}, {"ORA", &m::ABS, &m::ORA, 3, 4}, {"ASL", &m::ABS, &m::ASL, 3, 6}, {"SLO", &m::ABS, &m::SLO, 3, 6},
                    /*1*/{"BPL", &m::REL, &m::BPL, 2, 2}, {"ORA", &m::IDY, &m::ORA, 2, 5}, {"JAM", &m::IMP, &m::JAM, 1, 1}, {"SLO", &m::IDY, &m::SLO, 2, 8}, {"NOP", &m::ZPX, &m::NOP, 2, 4}, {"ORA", &m::ZPX, &m::ORA, 2, 4}, {"ASL", &m::ZPX, &m::ASL, 2, 6}, {"SLO", &m::ZPX, &m::SLO, 2, 6}, {"CLC", &m::IMP, &m::CLC, 1, 2}, {"ORA", &m::ABY, &m::ORA, 3, 4}, {"NOP", &m::IMP, &m::NOP, 1, 2}, {"SLO", &m::ABY, &m::SLO, 3, 7}, {"NOP", &m::ABX, &m::NOP, 3, 4}, {"ORA", &m::ABX, &m::ORA, 3, 4}, {"ASL", &m::ABX, &m::ASL, 3, 7}, {"SLO", &m::ABX, &m::SLO, 3, 7},
                    /*2*/{"JSR", &m::ABS, &m::JSR, 3, 6}, {"AND", &m::IDX, &m::AND, 2, 6}, {"JAM", &m::IMP, &m::JAM, 1, 1}, {"RLA", &m::IDX, &m::RLA, 2, 8}, {"BIT", &m::ZP0, &m::BIT, 2, 3}, {"AND", &m::ZP0, &m::AND, 2, 3}, {"ROL", &m::ZP0, &m::ROL, 2, 5}, {"RLA", &m::ZP0, &m::RLA, 2, 5}, {"PLP", &m::IMP, &m::PLP, 1, 4}, {"AND", &m::IMM, &m::AND, 2, 2}, {"ROL", &m::ACC, &m::ROL, 1, 2}, {"ANC", &m::IMM, &m::ANC, 2, 2}, {"BIT", &m::ABS, &m::BIT, 3, 4}, {"AND", &m::ABS, &m::AND, 3, 4}, {"ROL", &m::ABS, &m::ROL, 3, 6}, {"RLA", &m::ABS, &m::RLA, 3, 6},
                    /*3*/{"BMI", &m::REL, &m::BMI, 2, 2}, {"AND", &m::IDY, &m::AND, 2, 5}, {"JAM", &m::IMP, &m::JAM, 1, 1}, {"RLA", &m::IDY, &m::RLA, 2, 8}, {"NOP", &m::ZPX, &m::NOP, 2, 4}, {"AND", &m::ZPX, &m::AND, 2, 4}, {"ROL", &m::ZPX, &m::ROL, 2, 6}, {"RLA", &m::ZPX, &m::RLA, 2, 6}, {"SEC", &m::IMP, &m::SEC, 1, 2}, {"AND", &m::ABY, &m::AND, 3, 4}, {"NOP", &m::IMP, &m::NOP, 1, 2}, {"RLA", &m::ABY, &m::RLA, 3, 7}, {"NOP", &m::ABX, &m::NOP, 3, 4}, {"AND", &m::ABX, &m::AND, 3, 4}, {"ROL", &m::ABX, &m::ROL, 3, 7}, {"RLA", &m::ABX, &m::RLA, 3, 7},
                    /*4*/{"RTI", &m::IMP, &m::RTI, 1, 6}, {"EOR", &m::IDX, &m::EOR, 2, 6}, {"JAM", &m::IMP, &m::JAM, 1, 1}, {"SRE", &m::IDX, &m::SRE, 2, 8}, {"NOP", &m::ZP0, &m::NOP, 2, 3}, {"EOR", &m::ZP0, &m::EOR, 2, 3}, {"LSR", &m::ZP0, &m::LSR, 2, 5}, {"SRE", &m::ZP0, &m::SRE, 2, 5}, {"PHA", &m::IMP, &m::PHA, 1, 3}, {"EOR", &m::IMM, &m::EOR, 2, 2}, {"LSR", &m::ACC, &m::LSR, 1, 2}, {"ALR", &m::IMM, &m::ALR, 2, 2}, {"JMP", &m::ABS, &m::JMP, 3, 3}, {"EOR", &m::ABS, &m::EOR, 3, 4}, {"LSR", &m::ABS, &m::LSR, 3, 6}, {"SRE", &m::ABS, &m::SRE, 3, 6},
                    /*5*/{"BVC", &m::REL, &m::BVC, 2, 2}, {"EOR", &m::IDY, &m::EOR, 2, 5}, {"JAM", &m::IMP, &m::JAM, 1, 1}, {"SRE", &m::IDY, &m::SRE, 2, 8}, {"NOP", &m::ZPX, &m::NOP, 2, 4}, {"EOR", &m::ZPX, &m::EOR, 2, 4}, {"LSR", &m::ZPX, &m::LSR, 2, 6}, {"SRE", &m::ZPX, &m::SRE, 2, 6}, {"CLI", &m::IMP, &m::CLI, 1, 2}, {"EOR", &m::ABY, &m::EOR, 3, 4}, {"NOP", &m::IMP, &m::NOP, 1, 2}, {"SRE", &m::ABY, &m::SRE, 3, 7}, {"NOP", &m::ABX, &m::NOP, 3, 4}, {"EOR", &m::ABX, &m::EOR, 3, 4}, {"LSR", &m::ABX, &m::LSR, 3, 7}, {"SRE", &m::ABX, &m::SRE, 3, 7},
                    /*6*/{"RTS", &m::IMP, &m::RTS, 1, 6}, {"ADC", &m::IDX, &m::ADC, 2, 6}, {"JAM", &m::IMP, &m::JAM, 1, 1}, {"RRA", &m::IDX, &m::RRA, 2, 8}, {"NOP", &m::ZP0, &m::NOP, 2, 3}, {"ADC", &m::ZP0, &m::ADC, 2, 3}, {"ROR", &m::ZP0, &m::ROR, 2, 5}, {"RRA", &m::ZP0, &m::RRA, 2, 5}, {"PLA", &m::IMP, &m::PLA, 1, 4}, {"ADC", &m::IMM, &m::ADC, 2, 2}, {"ROR", &m::ACC, &m::ROR, 1, 2}, {"ARR", &m::IMM, &m::ARR, 2, 2}, {"JMP", &m::ID0, &m::JMP, 3, 5}, {"ADC", &m::ABS, &m::ADC, 3, 4}, {"ROR", &m::ABS, &m::ROR, 3, 6}, {"RRA", &m::ABS, &m::RRA, 3, 6},
                    /*7*/{"BVS", &m::REL, &m::BVS, 2, 2}, {"ADC", &m::IDY, &m::ADC, 2, 5}, {"JAM", &m::IMP, &m::JAM, 1, 1}, {"RRA", &m::IDY, &m::RRA, 2, 8}, {"NOP", &m::ZPX, &m::NOP, 2, 4}, {"ADC", &m::ZPX, &m::ADC, 2, 4}, {"ROR", &m::ZPX, &m::ROR, 2, 6}, {"RRA", &m::ZPX, &m::RRA, 2, 6}, {"SEI", &m::IMP, &m::SEI, 1, 2}, {"ADC", &m::ABY, &m::ADC, 3, 4}, {"NOP", &m::IMP, &m::NOP, 1, 2}, {"RRA", &m::ABY, &m::RRA, 3, 7}, {"NOP", &m::ABX, &m::NOP, 3, 4}, {"ADC", &m::ABX, &m::ADC, 3, 4}, {"ROR", &m::ABX, &m::ROR, 3, 7}, {"RRA", &m::ABX, &m::RRA, 3, 7},
                    /*8*/{"NOP", &m::IMM, &m::NOP, 2, 2}, {"STA", &m::IDX, &m::STA, 2, 6}, {"NOP", &m::IMM, &m::NOP, 2, 2}, {"SAX", &m::IDX, &m::SAX, 2, 6}, {"STY", &m::ZP0, &m::STY, 2, 3}, {"STA", &m::ZP0, &m::STA, 2, 3}, {"STX", &m::ZP0, &m::STX, 2, 3}, {"SAX", &m::ZP0, &m::SAX, 2, 3}, {"DEY", &m::IMP, &m::DEY, 1, 2}, {"NOP", &m::IMM, &m::NOP, 2, 2}, {"TXA", &m::IMP, &m::TXA, 1, 2}, {"ANE", &m::IMM, &m::ANE, 2, 2}, {"STY", &m::ABS, &m::STY, 3, 4}, {"STA", &m::ABS, &m::STA, 3, 4}, {"STX", &m::ABS, &m::STX, 3, 4}, {"SAX", &m::ABS, &m::SAX, 3, 4},
                    /*9*/{"BCC", &m::REL, &m::BCC, 2, 2}, {"STA", &m::IDY, &m::STA, 2, 6}, {"JAM", &m::IMP, &m::JAM, 1, 1}, {"SHA", &m::IDY, &m::SHA, 2, 6}, {"STY", &m::ZPX, &m::STY, 2, 4}, {"STA", &m::ZPX, &m::STA, 2, 4}, {"STX", &m::ZPY, &m::STX, 2, 4}, {"SAX", &m::ZPY, &m::SAX, 2, 4}, {"TYA", &m::IMP, &m::TYA, 1, 2}, {"STA", &m::ABY, &m::STA, 3, 5}, {"TXS", &m::IMP, &m::TXS, 1, 2}, {"TAS", &m::ABY, &m::TAS, 3, 5}, {"SHY", &m::ABX, &m::SHY, 3, 5}, {"STA", &m::ABX, &m::STA, 3, 5}, {"SHX", &m::ABY, &m::SHX, 3, 5}, {"SHA", &m::ABY, &m::SHA, 3, 5},
                    /*A*/{"LDY", &m::IMM, &m::LDY, 2, 2}, {"LDA", &m::IDX, &m::LDA, 2, 6}, {"LDX", &m::IMM, &m::LDX, 2, 2}, {"LAX", &m::IDX, &m::LAX, 2, 6}, {"LDY", &m::ZP0, &m::LDY, 2, 3}, {"LDA", &m::ZP0, &m::LDA, 2, 3}, {"LDX", &m::ZP0, &m::LDX, 2, 3}, {"LAX", &m::ZP0, &m::LAX, 2, 3}, {"TAY", &m::IMP, &m::TAY, 1, 2}, {"LDA", &m::IMM, &m::LDA, 2, 2}, {"TAX", &m::IMP, &m::TAX, 1, 2}, {"LXA", &m::IMM, &m::LXA, 2, 2}, {"LDY", &m::ABS, &m::LDY, 3, 4}, {"LDA", &m::ABS, &m::LDA, 3, 4}, {"LDX", &m::ABS, &m::LDX, 3, 4}, {"LAX", &m::ABS, &m::LAX, 3, 4},
                    /*B*/{"BCS", &m::REL, &m::BCS, 2, 2}, {"LDA", &m::IDY, &m::LDA, 2, 5}, {"JAM", &m::IMP, &m::JAM, 1, 1}, {"LAX", &m::IDY, &m::LAX, 2, 5}, {"LDY", &m::ZPX, &m::LDY, 2, 4}, {"LDA", &m::ZPX, &m::LDA, 2, 4}, {"LDX", &m::ZPY, &m::LDX, 2, 4}, {"LAX", &m::ZPY, &m::LAX, 2, 4}, {"CLV", &m::IMP, &m::CLV, 1, 2}, {"LDA", &m::ABY, &m::LDA, 3, 4}, {"TSX", &m::IMP, &m::TSX, 1, 2}, {"LAS", &m::ABY, &m::LAS, 3, 4}, {"LDY", &m::ABX, &m::LDY, 3, 4}, {"LDA", &m::ABX, &m::LDA, 3, 4}, {"LDX", &m::ABY, &m::LDX, 3, 4}, {"LAX", &m::ABY, &m::LAX, 3, 4},
                    /*C*/{"CPY", &m::IMM, &m::CPY, 2, 2}, {"CMP", &m::IDX, &m::CMP, 2, 6}, {"NOP", &m::IMM, &m::NOP, 2, 2}, {"DCP", &m::IDX, &m::DCP, 2, 8}, {"CPY", &m::ZP0, &m::CPY, 2, 3}, {"CMP", &m::ZP0, &m::CMP, 2, 3}, {"DEC", &m::ZP0, &m::DEC, 2, 5}, {"DCP", &m::ZP0, &m::DCP, 2, 5}, {"INY", &m::IMP, &m::INY, 1, 2}, {"CMP", &m::IMM, &m::CMP, 2, 2}, {"DEX", &m::IMP, &m::DEX, 1, 2}, {"SBX", &m::IMM, &m::SBX, 2, 2}, {"CPY", &m::ABS, &m::CPY, 3, 4}, {"CMP", &m::ABS, &m::CMP, 3, 4}, {"DEC", &m::ABS, &m::DEC, 3, 6}, {"DCP", &m::ABS, &m::DCP, 3, 6},
                    /*D*/{"BNE", &m::REL, &m::BNE, 2, 2}, {"CMP", &m::IDY, &m::CMP, 2, 5}, {"JAM", &m::IMP, &m::JAM, 1, 1}, {"DCP", &m::IDY, &m::DCP, 2, 8}, {"NOP", &m::ZPX, &m::NOP, 2, 4}, {"CMP", &m::ZPX, &m::CMP, 2, 4}, {"DEC", &m::ZPX, &m::DEC, 2, 6}, {"DCP", &m::ZPX, &m::DCP, 2, 6}, {"CLD", &m::IMP, &m::CLD, 1, 2}, {"CMP", &m::ABY, &m::CMP, 3, 4}, {"NOP", &m::IMP, &m::NOP, 1, 2}, {"DCP", &m::ABY, &m::DCP, 3, 7}, {"NOP", &m::ABX, &m::NOP, 3, 4}, {"CMP", &m::ABX, &m::CMP, 3, 4}, {"DEC", &m::ABX, &m::DEC, 3, 7}, {"DCP", &m::ABX, &m::DCP, 3, 7},
                    /*E*/{"CPX", &m::IMM, &m::CPX, 2, 2}, {"SBC", &m::IDX, &m::SBC, 2, 6}, {"NOP", &m::IMM, &m::NOP, 2, 2}, {"ISB", &m::IDX, &m::ISB, 2, 8}, {"CPX", &m::ZP0, &m::CPX, 2, 3}, {"SBC", &m::ZP0, &m::SBC, 2, 3}, {"INC", &m::ZP0, &m::INC, 2, 5}, {"ISB", &m::ZP0, &m::ISB, 2, 5}, {"INX", &m::IMP, &m::INX, 1, 2}, {"SBC", &m::IMM, &m::SBC, 2, 2}, {"NOP", &m::IMP, &m::NOP, 1, 2}, {"SBC", &m::IMM, &m::SBC, 2, 2}, {"CPX", &m::ABS, &m::CPX, 3, 4}, {"SBC", &m::ABS, &m::SBC, 3, 4}, {"INC", &m::ABS, &m::INC, 3, 6}, {"ISB", &m::ABS, &m::ISB, 3, 6},
                    /*F*/{"BEQ", &m::REL, &m::BEQ, 2, 2}, {"SBC", &m::IDY, &m::SBC, 2, 5}, {"JAM", &m::IMP, &m::JAM, 1, 1}, {"ISB", &m::IDY, &m::ISB, 2, 8}, {"NOP", &m::ZPX, &m::NOP, 2, 4}, {"SBC", &m::ZPX, &m::SBC, 2, 4}, {"INC", &m::ZPX, &m::INC, 2, 6}, {"ISB", &m::ZPX, &m::ISB, 2, 6}, {"SED", &m::IMP, &m::SED, 1, 2}, {"SBC", &m::ABY, &m::SBC, 3, 4}, {"NOP", &m::IMP, &m::NOP, 1, 2}, {"ISB", &m::ABY, &m::ISB, 3, 7}, {"NOP", &m::ABX, &m::NOP, 3, 4}, {"SBC", &m::ABX, &m::SBC, 3, 4}, {"INC", &m::ABX, &m::INC, 3, 7}, {"ISB", &m::ABX, &m::ISB, 3, 7}
            };

    /// Status register flags. Not using bit fields to ease external manipulation.
    struct status_flags_t {
        /// Carry.
        bool c = false;
        /// Zero.
        bool z = false;
        /// IRQ mask.
        bool i = false;
        /// Decimal mode.
        bool d = false;
        /// BRK command.
        bool b = false;
        /// Unused.
        bool x = false;
        /// Overflow.
        bool v = false;
        /// Negative.
        bool n = false;
    };

    /// Registers.
    struct {
        uint8_t x = 0x00;           //Index register X.
        uint8_t y = 0x00;           //Index register Y.
        status_flags_t status{};    //Status register.
        uint8_t acc = 0x00;         //Accumulator.
        uint8_t sp = 0x00;          //Stack pointer. The stack is a descending type.
        uint16_t pc = 0x0000;       //Program counter.
    } m_registers;

    // ===========================================
    // Emulation helper variables
    // ===========================================
    /// Absolute address.
    uint16_t m_addrAbs = false;
    /// Relative address.
    uint16_t m_addrRel = false;
    /// Currently remaining cycles.
    uint8_t m_cycles   = 0;
    /// All cycles.
    unsigned long long m_cycleCount = 0;
    /// Signalizes accumulator operation address mode.
    bool m_accOperation = false;

    /// Next CPU mode.
    enum class nextMode_t {
        /// Fetch next instruction.
        INSTRUCTION,
        /// Jump to NMI ISR.
        IRQ_ISR,
        /// Jump to IRQ ISR.
        NMI_ISR
    } m_next = nextMode_t::INSTRUCTION;

    /// NMI pin state.
    bool m_nmi = false;
    /// NMI pending state.
    bool m_nmiPending = false;
    /// IRQ pin state.
    bool m_irq = false;
    /// IRQ pending state.
    bool m_irqPending = false;
    /// Previous interrupt mask flag state.
    bool m_oldInterruptMask = false;

    /// Current instruction, initialized to NOP.
    instruction_t m_currentInstruction;
    /// Current opcode (instruction index).
    uint8_t m_currentOpcode = 0xEA;

    // ===========================================
    // Emulator internal functions
    // ===========================================
    /// Get current address mode description.
    std::string getCurrentAddressString();
    /**
     * Process an IRQ.
     */
    void irqHandler();
    /**
     * Process a NMI.
     * */
    void nmiHandler();
    /**
     * Do a hard reset.
     * This function sets the CPU to a default power-on state.
     * */
    void hardReset();

    // ===========================================
    // I/O
    // ===========================================
    /// Connection to the main system bus.
    DataPort m_mainBus;

    /// Interrupt request signal.
    void IRQ(bool active);
    /// Non-maskable interrupt signal.
    void NMI();
    /// Master clock.
    void CLK();

    // ===========================================
    // Pseudoinstructions
    // ===========================================
    /**
     * This function does common operation for all branching instructions.
     * */
    void branch(bool condition);

    // ===========================================
    // Addressing modes
    // ===========================================
    /** @defgroup addrModes Addressing modes.
     * Addressing mode functions.
     * Some instruction have variants according to different addresing modes.
     * The addresing mode behavior is the same across all instructions, so
     * the functions can be reused for every instruction.
     * @return n of additional required cycles, 0 otherwise.
     * @{
     */
    // Non-indexed, non memory.
    uint8_t ACC(); // Accumulator is the target.
    uint8_t IMM(); // Data is the next byte.
    uint8_t IMP(); // Instruction implies target.

    // Non-indexed memory access.
    uint8_t ABS(); // Absoulute mode.
    uint8_t ZP0(); // Zero-Page mode (first 256 B mapping).
    uint8_t REL(); // Relative mode (branching).
    uint8_t ID0(); // Indirect mode (like a pointer).

    // Indexed memory access.
    uint8_t ABX(); // Absolute indexed + offset in X register.
    uint8_t ABY(); // Absolute indexed + offset in Y register.
    uint8_t ZPX(); // Like ABX but limited to first 256 B.
    uint8_t ZPY(); // Like ABY but limited to first 256 B.
    uint8_t IDX(); // (Indirect,X).
    uint8_t IDY(); // (Indirect),Y.
    /** @} */

    // ===========================================
    // Instructions modes
    // ===========================================
    uint8_t ADC(); //!< Add with carry.
    uint8_t AND(); //!< Logical AND.
    uint8_t ASL(); //!< Arithmetic shift left.
    uint8_t BCC(); //!< Branch if carry clear.
    uint8_t BCS(); //!< Branch if carry set.
    uint8_t BEQ(); //!< Branch if equal.
    uint8_t BIT(); //!< Bit test.
    uint8_t BMI(); //!< Branch if minus.
    uint8_t BNE(); //!< Branch if not equal.
    uint8_t BPL(); //!< Branch if positive.
    uint8_t BRK(); //!< Force interrupt.
    uint8_t BVC(); //!< Branch if overflow clear.
    uint8_t BVS(); //!< Branch if overflow set.
    uint8_t CLC(); //!< Clear carry flag.
    uint8_t CLD(); //!< Clear decimal mode flag.
    uint8_t CLI(); //!< Clear interrupt disable flag.
    uint8_t CLV(); //!< Clear overflow flag.
    uint8_t CMP(); //!< Compare.
    uint8_t CPX(); //!< Compare X register.
    uint8_t CPY(); //!< Compare Y register.
    uint8_t DEC(); //!< Decrement memory.
    uint8_t DEX(); //!< Decrement X register.
    uint8_t DEY(); //!< Decrement Y register.
    uint8_t EOR(); //!< Exclusive OR.
    uint8_t INC(); //!< Increment memory.
    uint8_t INX(); //!< Increment X register.
    uint8_t INY(); //!< Increment Y register.
    uint8_t JMP(); //!< Modify the pc.
    uint8_t JSR(); //!< Jump to subroutine.
    uint8_t LDA(); //!< Load accumulator.
    uint8_t LDX(); //!< Load X register.
    uint8_t LDY(); //!< Load Y register.
    uint8_t LSR(); //!< Logical shift right.
    uint8_t NOP(); //!< No operation.
    uint8_t ORA(); //!< Logical inclusive OR.
    uint8_t PHA(); //!< Push accumulator.
    uint8_t PHP(); //!< Push processor status.
    uint8_t PLA(); //!< Pull accumulator.
    uint8_t PLP(); //!< Pull processor status.
    uint8_t ROL(); //!< Rotate left.
    uint8_t ROR(); //!< Rotate right.
    uint8_t RTI(); //!< Return from interrupt.
    uint8_t RTS(); //!< Return from subroutine.
    uint8_t SBC(); //!< Subtract with carry.
    uint8_t SEC(); //!< Set carry flag.
    uint8_t SED(); //!< Set decimal flag.
    uint8_t SEI(); //!< Set interrupt disable.
    uint8_t STA(); //!< Store accumulator.
    uint8_t STX(); //!< Store X register.
    uint8_t STY(); //!< Store Y register.
    uint8_t TAX(); //!< Transfer accumulator to X register.
    uint8_t TAY(); //!< Transfer accumulator to Y register.
    uint8_t TSX(); //!< Transfer stack pointer to X.
    uint8_t TXA(); //!< Transfer X to accumulator.
    uint8_t TXS(); //!< Transfer X to stack pointer.
    uint8_t TYA(); //!< Transfer Y to accumulator.

    // Illegal instructions.
    uint8_t ALR(); //!< (ASR) AND ; LSR.
    uint8_t ANC(); //!< AND ; bit 7 -> C (as in ASL/ROL)
    uint8_t ANE(); //!< (XAA) (A OR CONST) AND X AND oper -> A. IRL highly unstable, const usually 0x00, 0xFF, 0xEE... Here 0xFF.
    uint8_t ARR(); //!< AND ; ROR.
    uint8_t DCP(); //!< (DCM) DEC ; CMP.
    uint8_t ISB(); //!< (ISC, INS) INC ; SBC.
    uint8_t LAS(); //!< (LAR) M AND SP -> A, X, SP.
    uint8_t LAX(); //!< LDA ; LDX
    uint8_t LXA(); //!< (LAX imm) (A OR CONST) AND oper -> A -> X. IRL highly unstable, see ANE.
    uint8_t RLA(); //!< ROL ; AND.
    uint8_t RRA(); //!< ROR ; ADC.
    uint8_t SAX(); //!< (AXS, AAX) A AND X -> M.
    uint8_t SBX(); //!< (AXS, SAX) (A AND X) - oper -> X. Sets flags like CMP.
    uint8_t SHA(); //!< (AHX, AXA) A AND X AND (H+1) -> M. IRL unstable, sometimes AND (H+1) dropped, boundary crossing unreliable.
    uint8_t SHX(); //!< (A11, SXA, XAS) X AND (H+1) -> M. Unstable, see SHA.
    uint8_t SHY(); //!< (A11, SYA, SAY) Y AND (H+1) -> M. Unstable, see SHA.
    uint8_t SLO(); //!< (ASO) ASL ; ORA.
    uint8_t SRE(); //!< (LSE) LSR ; EOR.
    uint8_t TAS(); //!< (XAS, SHS) A AND X -> SP, A AND X AND (H+1) -> M. Unstable, see SHA.
    uint8_t JAM(); //!< (KIL, HLT) Jams the CPU.

public:

    explicit MOS6502();
    ~MOS6502() override;

    void init() override;

    std::vector<EmulatorWindow> getGUIs() override;

    void softReset();

    [[nodiscard]] bool instrFinished() const;
};

#endif //USE_6502_H
