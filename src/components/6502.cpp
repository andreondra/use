//
// Created by golas on 21.2.23.
//

#include "components/6502.h"
#include "Connector.h"
#include <memory>
#include <sstream>

MOS6502::MOS6502()
        : addrAbs(0), addrRel(0), cycles(0), cycleCount(0), accOperation(false),
          m_currentInstruction(lookup[0xEA]) {

    m_connectors["CLK"] = std::make_shared<Connector>(SignalInterface{
        .send = [this](){
            CLK();
        }
    });

    m_connectors["NMI"] = std::make_shared<Connector>(SignalInterface{
        .send = [this](){
            NMI();
        }
    });

    m_connectors["IRQ"] = std::make_shared<Connector>(SignalInterface{
        .send = [this](){
            IRQ();
        }
    });

    m_ports["mainBus"] = &m_mainBus;

    hardReset();
}

MOS6502::~MOS6502(){}

void MOS6502::branch(bool condition){

    if(condition){

        cycles++;

        if((addrAbs & 0xFF00) != (m_registers.pc & 0xFF00))
            cycles++;

        m_registers.pc = addrAbs; // Do not move above the 'if', old PC value is checked.
    }
}

std::string MOS6502::buildInstrString(instruction_t instr) {

    std::stringstream instrStr;
    instrStr << instr.mnemonic;

    if(instr.addrMode == &MOS6502::ACC)
        instrStr << " A";
    else if(instr.addrMode == &MOS6502::IMM)
        instrStr << " #$" << std::hex << (int)m_mainBus.read(m_registers.pc - 1) << " (IMM)";
    else if(instr.addrMode == &MOS6502::ABS)
        instrStr << " $" << std::hex << addrAbs << " (ABS)";
    else if(instr.addrMode == &MOS6502::ZP0)
        instrStr << " $" << std::hex << addrAbs << " (ZP0)";
    else if(instr.addrMode == &MOS6502::REL)
        instrStr << " $" << std::hex << addrRel << " (REL)";
    else if(instr.addrMode == &MOS6502::ID0)
        instrStr << " ($" << std::hex <<  addrAbs << ")" << " (ID0)";
    else if(instr.addrMode == &MOS6502::ABX)
        instrStr << " $" << std::hex << addrAbs << ",X" << " (ABX)";
    else if(instr.addrMode == &MOS6502::ABY)
        instrStr << " $" << std::hex << addrAbs << ",Y" << " (ABY)";
    else if(instr.addrMode == &MOS6502::ZPX)
        instrStr << " $" << std::hex << addrAbs << ",X" << " (ZPX)";
    else if(instr.addrMode == &MOS6502::ZPY)
        instrStr << " $" << std::hex << addrAbs << ",Y" << " (ZPY)";
    else if(instr.addrMode == &MOS6502::IDX)
        instrStr << " ($" << std::hex << addrAbs << ",X)" << " (IDX)";
    else if(instr.addrMode == &MOS6502::IDY)
        instrStr << " ($" << std::hex << addrAbs << "),Y" << " (IDY)";
    else if(instr.addrMode == &MOS6502::IMP)
        instrStr << " (IMP)";
    else
        instrStr << " (\?\?\?)";

    return instrStr.str();
}

void MOS6502::hardReset() {

    // Initial status NVxBDIZC = 0x34
    m_registers.status.c = 0;
    m_registers.status.z = 0;
    m_registers.status.i = 1;
    m_registers.status.d = 0;
    m_registers.status.b = 1;
    m_registers.status.x = 1;
    m_registers.status.n = 0;
    m_registers.status.v = 0;

    m_registers.acc = 0;
    m_registers.x = 0;
    m_registers.y = 0;
    m_registers.sp = 0xFD;

    for(uint16_t i = 0; i <= 0x7FF; i++){
        m_mainBus.write(i, 0x00);
    }
    for(uint16_t i = 0; i <= 0xF; i++){
        m_mainBus.write(0x4000 + i, 0x00);
    }
    uint16_t addrToClean[] = {0x4017, 0x4015, 0x4010, 0x4011, 0x4012, 0x4013};
    for(uint16_t i = 0; i < 6; i++){
        m_mainBus.write(addrToClean[i], 0x00);
    }

    m_registers.pc = m_mainBus.read(0xFFFC) | ((uint16_t)m_mainBus.read(0xFFFD) << 8);

    addrAbs = addrRel = accOperation = 0;
    cycles = 7;
    cycleCount = 0;
    irqEligible = false;
    irqPending = false;
    nmiPending = false;

    m_currentOpcode = 0xEA;
    m_currentInstruction = lookup[m_currentOpcode];
}

void MOS6502::softReset(){

    m_registers.sp -= 3;
    m_registers.status.i = 1;
    m_mainBus.write(0x4015, 0x00);

    m_registers.pc = m_mainBus.read(0xFFFC) | ((uint16_t)m_mainBus.read(0xFFFD) << 8);

    addrAbs = addrRel = accOperation = 0;
    cycles = 7;
    cycleCount = 0;

    m_currentInstruction = lookup[0xEA];
}

void MOS6502::IRQ(){

    irqPending = true;
}

void MOS6502::irqHandler(){

    m_mainBus.write(stackPos + m_registers.sp, (uint8_t)((m_registers.pc & 0xFF00) >> 8));
    m_registers.sp--;
    m_mainBus.write(stackPos + m_registers.sp, (uint8_t)(m_registers.pc & 0xFF));
    m_registers.sp--;

    uint8_t status = m_registers.status.c;
    status |= m_registers.status.z << 1;
    status |= m_registers.status.i << 2;
    status |= m_registers.status.d << 3;
    status |= 0x0 << 4;
    status |= 0x1 << 5; //Status 5 always 1.
    status |= m_registers.status.v << 6;
    status |= m_registers.status.n << 7;
    m_mainBus.write(stackPos + m_registers.sp, status);
    m_registers.sp--;

    m_registers.pc = m_mainBus.read(0xFFFE) | ((uint16_t)m_mainBus.read(0xFFFF) << 8);

    m_registers.status.i = 1;

    cycles += 7;
}

void MOS6502::NMI(){
    nmiPending = true;
}

void MOS6502::nmiHandler(){

    m_mainBus.write(stackPos + m_registers.sp, (uint8_t)((m_registers.pc & 0xFF00) >> 8));
    m_registers.sp--;
    m_mainBus.write(stackPos + m_registers.sp, (uint8_t)(m_registers.pc & 0xFF));
    m_registers.sp--;

    uint8_t status = m_registers.status.c;
    status |= m_registers.status.z << 1;
    status |= m_registers.status.i << 2;
    status |= m_registers.status.d << 3;
    status |= 0x0 << 4;
    status |= 0x1 << 5; //Status 5 always 1.
    status |= m_registers.status.v << 6;
    status |= m_registers.status.n << 7;
    m_mainBus.write(stackPos + m_registers.sp, status);
    m_registers.sp--;

    m_registers.pc = m_mainBus.read(0xFFFA) | ((uint16_t)m_mainBus.read(0xFFFB) << 8);

    m_registers.status.i = 1;

    cycles += 7;
}

void MOS6502::CLK(){

    if(cycles == 0){

        irqEligible = !m_registers.status.i;

        m_currentOpcode = m_mainBus.read(m_registers.pc++);
        m_currentInstruction = lookup[m_currentOpcode];
        uint8_t addrRet = (this->*(m_currentInstruction.addrMode))();
        uint8_t instrRet = (this->*(m_currentInstruction.instrCode))();

        if(
            //opcode == 0x40  // RTI

                m_currentOpcode != 0x78 && // SEI
                m_currentOpcode != 0x58 && // CLI
                m_currentOpcode != 0x28    // PLP
                ){
            irqEligible = !m_registers.status.i;
        }

        if(nmiPending){

            if(m_currentOpcode == 0x00)
                m_registers.pc = m_mainBus.read(0xFFFA) | ((uint16_t)m_mainBus.read(0xFFFB) << 8);
            else
                nmiHandler();

            nmiPending = false;
            irqPending = false;
        } else if(irqPending && irqEligible) {

            irqHandler();
            irqPending = irqEligible = false;
        }

        cycles += m_currentInstruction.cycles;
        if (addrRet && instrRet) cycles++;
    }

    cycles--;
    cycleCount++;
}

bool MOS6502::instrFinished(){
    return cycles == 0;
}

const char * MOS6502::currentMnemonic() const{
    return m_currentInstruction.mnemonic;
}

std::string MOS6502::currentInstruction() {

    return buildInstrString(m_currentInstruction);
}

std::string MOS6502::nextInstruction() {

    uint8_t opcode = m_mainBus.read(m_registers.pc);
    return buildInstrString(lookup[opcode]);
}

std::string MOS6502::nextOpcode() {
    return lookup[m_mainBus.read(m_registers.pc)].mnemonic;
}

uint64_t MOS6502::getCycleCount() const{
    return cycleCount;
}

uint8_t MOS6502::getRemainingCycles() const{
    return cycles;
}

std::vector<EmulatorWindow> MOS6502::getGUIs() {

    return {};
}
// =========================================================================================
//Addressing modes.
uint8_t MOS6502::ACC(){
    accOperation = true;
    return 0;
}

uint8_t MOS6502::IMP(){

    return 0;
}

uint8_t MOS6502::IMM(){

    addrAbs = m_registers.pc++;
    return 0;
}

uint8_t MOS6502::ABS(){

    addrAbs = m_mainBus.read(m_registers.pc) | (m_mainBus.read(m_registers.pc + 1) << 8);
    m_registers.pc += 2;
    return 0;
}

uint8_t MOS6502::ZP0(){

    addrAbs = m_mainBus.read(m_registers.pc) & 0x00FF;
    m_registers.pc++;
    return 0;
}

uint8_t MOS6502::REL(){

    addrRel = m_mainBus.read(m_registers.pc);
    m_registers.pc++;

    if(addrRel & 0x80) //If the byte was negative, make whole addr negative.
        addrRel |= 0xFF00;

    addrAbs = m_registers.pc + addrRel;

    return 0;
}

uint8_t MOS6502::ID0(){

    addrRel = m_mainBus.read(m_registers.pc) | (m_mainBus.read(m_registers.pc + 1) << 8);

    //HW bug implementation. If the lo byte of pointer is 0xFF,
    //CPU wraps back to the same page.
    if((addrRel & 0x00FF) == 0x00FF)
        addrAbs = m_mainBus.read(addrRel) | (m_mainBus.read(addrRel & 0xFF00) << 8);
    else
        addrAbs = m_mainBus.read(addrRel) | (m_mainBus.read(addrRel + 1) << 8);

    //pc increment is not needed, JMP will change pc anyways.
    return 0;
}

uint8_t MOS6502::ABX(){

    addrRel = m_mainBus.read(m_registers.pc) | (m_mainBus.read(m_registers.pc + 1) << 8);
    addrAbs = addrRel + m_registers.x;
    m_registers.pc += 2;

    if((addrRel & 0xFF00) == (addrAbs & 0xFF00))
        return 0;
    else
        return 1;
}

uint8_t MOS6502::ABY(){

    addrRel = m_mainBus.read(m_registers.pc) | (m_mainBus.read(m_registers.pc + 1) << 8);
    addrAbs = addrRel + m_registers.y;
    m_registers.pc += 2;

    if((addrRel & 0xFF00) == (addrAbs & 0xFF00))
        return 0;
    else
        return 1;
}

uint8_t MOS6502::ZPX(){

    addrRel = m_mainBus.read(m_registers.pc);
    addrAbs = (addrRel + m_registers.x) & 0x00FF;
    m_registers.pc++;

    return 0;
}

uint8_t MOS6502::ZPY(){

    addrRel = m_mainBus.read(m_registers.pc);
    addrAbs = (addrRel + m_registers.y) & 0x00FF;
    m_registers.pc++;

    return 0;
}

uint8_t MOS6502::IDX(){

    addrRel = (m_mainBus.read(m_registers.pc) + m_registers.x);
    addrAbs = m_mainBus.read(addrRel & 0x00FF) | (m_mainBus.read((addrRel + 1) & 0x00FF) << 8);
    m_registers.pc++;

    return 0;
}

uint8_t MOS6502::IDY(){

    addrRel = m_mainBus.read(m_registers.pc);
    uint16_t newAddr = (m_mainBus.read(addrRel) | ((uint16_t)m_mainBus.read((addrRel + 1) & 0x00FF) << 8));
    addrAbs = newAddr + m_registers.y;
    m_registers.pc++;

    if((addrAbs & 0xFF00) == (newAddr & 0xFF00))
        return 0;
    else
        return 1;
}

// =========================================================================================
//Instructions.

uint8_t MOS6502::ADC(){

    uint8_t memoryValue = m_mainBus.read(addrAbs);

    bool memoryNegative = (memoryValue & 0x80) == 0x80;
    bool accNegative = (m_registers.acc & 0x80) == 0x80;

    uint16_t result = (uint16_t)m_registers.acc + (uint16_t)memoryValue + (uint16_t)m_registers.status.c;
    m_registers.status.c = (result & 0x100) == 0x100;
    result &= 0xFF;
    m_registers.status.z = result == 0x0;
    m_registers.status.n = (result & 0x80) == 0x80;

    if(memoryNegative != accNegative)
        m_registers.status.v = 0;
    else
        m_registers.status.v = memoryNegative == accNegative && memoryNegative != m_registers.status.n;

    m_registers.acc = result;

    return 1;
}

uint8_t MOS6502::AND(){

    m_registers.acc = m_registers.acc & m_mainBus.read(addrAbs);
    m_registers.status.z = m_registers.acc == 0x0;
    m_registers.status.n = (m_registers.acc & 0x80) == 0x80;

    return 1;
}

uint8_t MOS6502::ASL(){

    if(accOperation){

        m_registers.status.c = (m_registers.acc & 0x80) >> 7;
        m_registers.acc <<= 1;
        m_registers.status.z = m_registers.acc == 0x0;
        m_registers.status.n = (m_registers.acc & 0x80) == 0x80;
        accOperation = false;
    } else {

        uint8_t value = m_mainBus.read(addrAbs);
        m_registers.status.c = (value & 0x80) >> 7;
        value <<= 1;
        m_registers.status.z = value == 0x0;
        m_registers.status.n = (value & 0x80) == 0x80;
        m_mainBus.write(addrAbs, value);
    }

    return 0;
}

uint8_t MOS6502::BCC(){

    branch(!m_registers.status.c);
    return 0;
}

uint8_t MOS6502::BCS(){

    branch(m_registers.status.c);
    return 0;
}

uint8_t MOS6502::BEQ(){

    branch(m_registers.status.z);
    return 0;
}

uint8_t MOS6502::BIT(){

    uint8_t value = m_mainBus.read(addrAbs);

    m_registers.status.n = (value & 0x80) >> 7;
    m_registers.status.v = (value & 0x40) >> 6;
    m_registers.status.z = (value & m_registers.acc) == 0x0;

    return 0;
}

uint8_t MOS6502::BMI(){

    branch(m_registers.status.n);
    return 0;
}

uint8_t MOS6502::BNE(){

    branch(!m_registers.status.z);
    return 0;
}

uint8_t MOS6502::BPL(){

    branch(!m_registers.status.n);
    return 0;
}

uint8_t MOS6502::BRK(){

    m_registers.pc++;

    m_mainBus.write(stackPos + m_registers.sp, (m_registers.pc & 0xFF00) >> 8);
    m_registers.sp--;
    m_mainBus.write(stackPos + m_registers.sp, m_registers.pc & 0xFF);
    m_registers.sp--;

    uint8_t status = m_registers.status.c;
    status |= m_registers.status.z << 1;
    status |= m_registers.status.i << 2;
    status |= m_registers.status.d << 3;
    status |= 0x1 << 4;
    status |= 0x1 << 5; //Status 5 always 1.
    status |= m_registers.status.v << 6;
    status |= m_registers.status.n << 7;
    m_mainBus.write(stackPos + m_registers.sp, status);
    m_registers.sp--;

    m_registers.status.i = 1;

    m_registers.pc = m_mainBus.read(0xFFFE) | ((uint16_t)m_mainBus.read(0xFFFF) << 8);

    return 0;
}

uint8_t MOS6502::BVC(){

    branch(!m_registers.status.v);
    return 0;
}

uint8_t MOS6502::BVS(){

    branch(m_registers.status.v);
    return 0;
}

uint8_t MOS6502::CLC(){

    m_registers.status.c = 0;
    return 0;
}

uint8_t MOS6502::CLD(){

    m_registers.status.d = 0;
    return 0;
}

uint8_t MOS6502::CLI(){

    m_registers.status.i = 0;
    return 0;
}

uint8_t MOS6502::CLV(){

    m_registers.status.v = 0;
    return 0;
}

uint8_t MOS6502::CMP(){

    uint8_t data = m_mainBus.read(addrAbs);
    m_registers.status.c = (m_registers.acc >= data);
    m_registers.status.z = ((m_registers.acc & 0x00FF) == data);
    m_registers.status.n = (((m_registers.acc - data) & 0x80) == 0x80);
    return 1;
}

uint8_t MOS6502::CPX(){

    uint8_t data = m_mainBus.read(addrAbs);
    m_registers.status.c = (m_registers.x >= data);
    m_registers.status.z = ((m_registers.x & 0x00FF) == data);
    m_registers.status.n = (((m_registers.x - data) & 0x80) == 0x80);
    return 1;
}

uint8_t MOS6502::CPY(){

    uint8_t data = m_mainBus.read(addrAbs);
    m_registers.status.c = (m_registers.y >= data);
    m_registers.status.z = (m_registers.y == data);
    m_registers.status.n = (((m_registers.y - data) & 0x80) == 0x80);
    return 1;
}

uint8_t MOS6502::DEC(){

    uint8_t newVal = m_mainBus.read(addrAbs) - 1;
    m_mainBus.write(addrAbs, newVal);

    m_registers.status.z = newVal == 0;
    m_registers.status.n = (newVal & 0x80) == 0x80;

    return 0;
}

uint8_t MOS6502::DEX(){

    m_registers.x--;

    m_registers.status.z = m_registers.x == 0;
    m_registers.status.n = (m_registers.x & 0x80) == 0x80;

    return 0;
}

uint8_t MOS6502::DEY(){

    uint8_t newVal = --m_registers.y;

    m_registers.status.z = newVal == 0;
    m_registers.status.n = (newVal & 0x80) == 0x80;

    return 0;
}

uint8_t MOS6502::EOR(){

    m_registers.acc = m_mainBus.read(addrAbs) ^ m_registers.acc;

    m_registers.status.z = m_registers.acc == 0;
    m_registers.status.n = (m_registers.acc & 0x80) == 0x80;
    return 1;
}

uint8_t MOS6502::INC(){

    uint8_t newVal = m_mainBus.read(addrAbs) + 1;
    m_mainBus.write(addrAbs, newVal);

    m_registers.status.z = newVal == 0;
    m_registers.status.n = (newVal & 0x80) == 0x80;
    return 0;
}

uint8_t MOS6502::INX(){

    m_registers.x++;

    m_registers.status.z = m_registers.x == 0;
    m_registers.status.n = (m_registers.x & 0x80) == 0x80;

    return 0;
}

uint8_t MOS6502::INY(){

    m_registers.y++;

    m_registers.status.z = m_registers.y == 0;
    m_registers.status.n = (m_registers.y & 0x80) == 0x80;

    return 0;
}

uint8_t MOS6502::JMP(){

    m_registers.pc = addrAbs;
    return 0;
}

uint8_t MOS6502::JSR(){

    m_registers.pc--;

    m_mainBus.write(stackPos + m_registers.sp, (m_registers.pc >> 8) & 0x00FF);
    m_registers.sp--;
    m_mainBus.write(stackPos + m_registers.sp, m_registers.pc & 0x00FF);
    m_registers.sp--;

    m_registers.pc = addrAbs;
    return 0;
}

uint8_t MOS6502::LDA(){

    m_registers.acc = m_mainBus.read(addrAbs);

    m_registers.status.z = m_registers.acc == 0;
    m_registers.status.n = (m_registers.acc & 0x80) == 0x80;

    return 1;
}

uint8_t MOS6502::LDX(){

    m_registers.x = m_mainBus.read(addrAbs);

    m_registers.status.z = m_registers.x == 0;
    m_registers.status.n = (m_registers.x & 0x80) == 0x80;

    return 1;
}

uint8_t MOS6502::LDY(){

    m_registers.y = m_mainBus.read(addrAbs);

    m_registers.status.z = m_registers.y == 0;
    m_registers.status.n = (m_registers.y & 0x80) == 0x80;

    return 1;
}

uint8_t MOS6502::LSR(){

    if(accOperation){
        m_registers.status.c = m_registers.acc & 0x1;
        m_registers.acc >>= 1;
        m_registers.status.z = m_registers.acc == 0;
        m_registers.status.n = (m_registers.acc & 0x80) == 0x80;
        accOperation = false;
    } else {
        m_registers.status.c = m_mainBus.read(addrAbs) & 0x1;
        uint8_t newVal = m_mainBus.read(addrAbs) >> 1;
        m_mainBus.write(addrAbs, newVal);
        m_registers.status.z = newVal == 0;
        m_registers.status.n = (newVal & 0x80) == 0x80;
    }

    return 0;
}

uint8_t MOS6502::NOP(){

    return 1;
}

uint8_t MOS6502::ORA(){

    m_registers.acc |= m_mainBus.read(addrAbs);
    m_registers.status.z = m_registers.acc == 0;
    m_registers.status.n = (m_registers.acc & 0x80) == 0x80;

    return 1;
}

uint8_t MOS6502::PHA(){

    m_mainBus.write(stackPos + m_registers.sp, m_registers.acc);
    m_registers.sp--;
    return 0;
}

uint8_t MOS6502::PHP(){

    uint8_t status = m_registers.status.c;
    status |= m_registers.status.z << 1;
    status |= m_registers.status.i << 2;
    status |= m_registers.status.d << 3;
    status |= 0x1 << 4;
    status |= 0x1 << 5; //Status 5 always 1.
    status |= m_registers.status.v << 6;
    status |= m_registers.status.n << 7;

    m_mainBus.write(stackPos + m_registers.sp, status);
    m_registers.sp--;
    return 0;
}

uint8_t MOS6502::PLA(){

    m_registers.sp++;
    m_registers.acc = m_mainBus.read(stackPos + m_registers.sp);
    m_registers.status.z = m_registers.acc == 0;
    m_registers.status.n = (m_registers.acc & 0x80) == 0x80;

    return 0;
}

uint8_t MOS6502::PLP(){

    m_registers.sp++;
    uint8_t status = m_mainBus.read(stackPos + m_registers.sp);
    m_registers.status.c = status & 0x1;
    m_registers.status.z = (status & 0x2) >> 1;
    m_registers.status.i = (status & 0x4) >> 2;
    m_registers.status.d = (status & 0x8) >> 3;
    m_registers.status.b = (status & 0x10) >> 4;
    m_registers.status.v = (status & 0x40) >> 6;
    m_registers.status.n = (status & 0x80) >> 7;

    return 0;
}

uint8_t MOS6502::ROL(){

    uint8_t oldCarry = m_registers.status.c;
    // Accumulator operation.
    if(accOperation){

        m_registers.status.c = (m_registers.acc & 0x80) >> 7;
        m_registers.acc <<= 1;
        m_registers.acc |= oldCarry;
        m_registers.status.n = (m_registers.acc & 0x80) == 0x80;
        m_registers.status.z = m_registers.acc == 0x0;

        accOperation = false;

        // Memory operation.
    } else {

        uint8_t value = m_mainBus.read(addrAbs);

        m_registers.status.c = (value & 0x80) >> 7;
        value <<= 1;
        value |= oldCarry;
        m_registers.status.n = (value & 0x80) == 0x80;
        m_registers.status.z = value == 0x0;

        m_mainBus.write(addrAbs, value);
    }

    return 0;
}

uint8_t MOS6502::ROR(){

    uint8_t oldCarry = m_registers.status.c << 7;
    // Accumulator operation.
    if(accOperation){

        m_registers.status.c = (m_registers.acc & 0x1);
        m_registers.acc >>= 1;
        m_registers.acc |= oldCarry;
        m_registers.status.n = (m_registers.acc & 0x80) == 0x80;
        m_registers.status.z = m_registers.acc == 0x0;

        accOperation = false;
        // Memory operation.
    } else {

        uint8_t value = m_mainBus.read(addrAbs);

        m_registers.status.c = (value & 0x1);
        value >>= 1;
        value |= oldCarry;
        m_registers.status.n = (value & 0x80) == 0x80;
        m_registers.status.z = value == 0x0;

        m_mainBus.write(addrAbs, value);
    }

    return 0;
}

uint8_t MOS6502::RTI(){

    m_registers.sp++;
    uint8_t flags = m_mainBus.read(stackPos + m_registers.sp);
    m_registers.sp++;
    m_registers.pc = m_mainBus.read(stackPos + m_registers.sp) | (m_mainBus.read(stackPos + m_registers.sp + 1) << 8);
    m_registers.sp++;

    m_registers.status.c = flags & 0x1;
    m_registers.status.z = (flags & 0x2) >> 1;
    m_registers.status.i = (flags & 0x4) >> 2;
    m_registers.status.d = (flags & 0x8) >> 3;
    m_registers.status.b = 0;
    m_registers.status.x = 0;
    m_registers.status.v = (flags & 0x40) >> 6;
    m_registers.status.n = (flags & 0x80) >> 7;

    return 0;
}

uint8_t MOS6502::RTS(){

    m_registers.sp++;
    m_registers.pc = ((m_mainBus.read(stackPos + m_registers.sp) | ((uint16_t)m_mainBus.read(stackPos + m_registers.sp + 1) << 8))) + 1;
    m_registers.sp++;

    return 0;
}

uint8_t MOS6502::SBC(){

    uint16_t memoryValue = (uint8_t)(~m_mainBus.read(addrAbs));

    uint16_t result = (uint16_t)m_registers.acc + memoryValue + (uint16_t)(m_registers.status.c);
    m_registers.status.c = result & 0xFF00;
    result &= 0xFF;
    m_registers.status.n = (result & 0x80) == 0x80;
    m_registers.status.z = result == 0x0;
    m_registers.status.v = (result ^ (uint16_t)m_registers.acc) & (result ^ memoryValue) & 0x0080;

    m_registers.acc = result;

    return 1;
}

uint8_t MOS6502::SEC(){

    m_registers.status.c = 1;
    return 0;
}

uint8_t MOS6502::SED(){

    m_registers.status.d = 1;
    return 0;
}

uint8_t MOS6502::SEI(){

    m_registers.status.i = 1;
    return 0;
}

uint8_t MOS6502::STA(){

    m_mainBus.write(addrAbs, m_registers.acc);
    return 0;
}

uint8_t MOS6502::STX(){

    m_mainBus.write(addrAbs, m_registers.x);
    return 0;
}

uint8_t MOS6502::STY(){

    m_mainBus.write(addrAbs, m_registers.y);
    return 0;
}

uint8_t MOS6502::TAX(){

    m_registers.x = m_registers.acc;
    m_registers.status.z = m_registers.x == 0x0;
    m_registers.status.n = (m_registers.x & 0x80) == 0x80;
    return 0;
}

uint8_t MOS6502::TAY(){

    m_registers.y = m_registers.acc;
    m_registers.status.z = m_registers.y == 0x0;
    m_registers.status.n = (m_registers.y & 0x80) == 0x80;
    return 0;
}

uint8_t MOS6502::TSX(){

    m_registers.x = m_registers.sp;
    m_registers.status.z = m_registers.x == 0x0;
    m_registers.status.n = (m_registers.x & 0x80) == 0x80;
    return 0;
}

uint8_t MOS6502::TXA(){

    m_registers.acc = m_registers.x;
    m_registers.status.z = m_registers.acc == 0x0;
    m_registers.status.n = (m_registers.acc & 0x80) == 0x80;
    return 0;
}

uint8_t MOS6502::TXS(){

    m_registers.sp = m_registers.x;
    return 0;
}

uint8_t MOS6502::TYA(){

    m_registers.acc = m_registers.y;
    m_registers.status.z = m_registers.acc == 0x0;
    m_registers.status.n = (m_registers.acc & 0x80) == 0x80;
    return 0;
}

// Illegal instructions.

uint8_t MOS6502::ALR(){

    AND();
    LSR();

    return 0;
}

uint8_t MOS6502::ANC(){

    AND();
    m_registers.status.c = (m_mainBus.read(addrAbs) & 0x80) >> 7;

    return 0;
}

uint8_t MOS6502::ANE(){

    m_registers.acc |= 0xFF;
    m_registers.acc &= m_registers.x;
    m_registers.acc &= m_mainBus.read(addrAbs);

    m_registers.status.n = (m_registers.acc & 0x80) == 0x80;
    m_registers.status.z = m_registers.acc == 0x0;

    return 0;
}

uint8_t MOS6502::ARR(){

    AND();
    ACC(); // Because ROR is performed on A.
    ROR();
    m_registers.status.c = (m_registers.acc & 0x40) >> 6;
    m_registers.status.v = ((m_registers.acc & 0x40) >> 6) ^ ((m_registers.acc & 0x20) >> 5);

    return 0;
}

uint8_t MOS6502::DCP(){

    DEC();
    CMP();
    return 0;
}

uint8_t MOS6502::ISB(){

    INC();
    SBC();
    return 0;
}

uint8_t MOS6502::LAS(){

    uint8_t value = m_mainBus.read(addrAbs) & m_registers.sp;
    m_registers.acc = value;
    m_registers.x = value;
    m_registers.sp = value;

    m_registers.status.n = (value & 0x80) == 0x80;
    m_registers.status.z = value == 0x0;

    return 1;
}

uint8_t MOS6502::LAX(){

    LDA();
    LDX();

    return 1;
}

uint8_t MOS6502::LXA(){

    m_registers.acc |= 0xFF;
    m_registers.acc &= m_mainBus.read(addrAbs);
    m_registers.x = m_registers.acc;

    m_registers.status.n = (m_registers.acc & 0x80) == 0x80;
    m_registers.status.z = m_registers.acc == 0x0;

    return 0;
}

uint8_t MOS6502::RLA(){

    ROL();
    AND();
    return 0;
}

uint8_t MOS6502::RRA(){

    ROR();
    ADC();
    return 0;
}

uint8_t MOS6502::SAX(){

    m_mainBus.write(addrAbs, m_registers.acc & m_registers.x);
    return 0;
}

uint8_t MOS6502::SBX(){

    m_registers.x = (m_registers.acc & m_registers.x) - m_mainBus.read(addrAbs);
    m_registers.status.n = (m_registers.x & 0x80) == 0x80;
    m_registers.status.z = m_registers.x == 0x0;
    m_registers.status.c = (m_registers.acc >= m_registers.x);
    return 0;
}

uint8_t MOS6502::SHA(){

    m_mainBus.write(addrAbs, m_registers.acc & m_registers.x & ((addrAbs & 0xFF00 >> 8) + 1));
    return 0;
}

uint8_t MOS6502::SHX(){

    m_mainBus.write(addrAbs, m_registers.x & (((addrRel & 0xFF00) >> 8) + 1));
    return 0;
}

uint8_t MOS6502::SHY(){

    m_mainBus.write(addrAbs, m_registers.y & (((addrRel & 0xFF00) >> 8) + 1));
    return 0;
}

uint8_t MOS6502::SLO(){

    ASL();
    ORA();
    return 0;
}

uint8_t MOS6502::SRE(){

    LSR();
    EOR();
    return 0;
}

uint8_t MOS6502::TAS(){

    m_registers.sp = m_registers.acc & m_registers.x;
    m_mainBus.write(addrAbs, m_registers.acc & m_registers.x & (((addrAbs & 0xFF00) >> 8) + 1));

    return 0;
}

uint8_t MOS6502::JAM(){

    m_registers.pc--; //Decreases the PC to the itself, effectively stopping the execution process.
    return 0;
}

void MOS6502::init() {
    hardReset();
}


