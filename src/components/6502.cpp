//
// Created by golas on 21.2.23.
//

#include "components/6502.h"
#include <sstream>

MOS6502::MOS6502()
        : addrAbs(0), addrRel(0), cycles(0), cycleCount(0), accOperation(false),
          m_currentInstruction(lookup[0xEA]) {

    m_ports["mainBus"] = &m_mainBus;
    hardReset();
}

MOS6502::~MOS6502(){}

void MOS6502::branch(bool condition){

    if(condition){

        cycles++;

        if((addrAbs & 0xFF00) != (registers.pc & 0xFF00))
            cycles++;

        registers.pc = addrAbs; // Do not move above the 'if', old PC value is checked.
    }
}

std::string MOS6502::buildInstrString(instruction_t instr) const{

    std::stringstream instrStr;
    instrStr << instr.mnemonic;

    if(instr.addrMode == &MOS6502::ACC)
        instrStr << " A";
    else if(instr.addrMode == &MOS6502::IMM)
        instrStr << " #$" << std::hex << (int)busRead(registers.pc - 1) << " (IMM)";
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
    registers.status.c = 0;
    registers.status.z = 0;
    registers.status.i = 1;
    registers.status.d = 0;
    registers.status.b = 1;
    registers.status.x = 1;
    registers.status.n = 0;
    registers.status.v = 0;

    registers.acc = 0;
    registers.x = 0;
    registers.y = 0;
    registers.sp = 0xFD;

    for(uint16_t i = 0; i <= 0x7FF; i++){
        busWrite(i, 0x00);
    }
    for(uint16_t i = 0; i <= 0xF; i++){
        busWrite(0x4000 + i, 0x00);
    }
    uint16_t addrToClean[] = {0x4017, 0x4015, 0x4010, 0x4011, 0x4012, 0x4013};
    for(uint16_t i = 0; i < 6; i++){
        busWrite(addrToClean[i], 0x00);
    }

    registers.pc = busRead(0xFFFC) | ((uint16_t)busRead(0xFFFD) << 8);

    addrAbs = addrRel = accOperation = 0;
    cycles = 7;
    cycleCount = 0;
    irqEligible = false;
    irqPending = false;
    nmiPending = false;
    m_hijackPending = false;

    m_currentOpcode = 0xEA;
    m_currentInstruction = lookup[m_currentOpcode];
}

void MOS6502::softReset(){

    registers.sp -= 3;
    registers.status.i = 1;
    busWrite(0x4015, 0x00);

    registers.pc = busRead(0xFFFC) | ((uint16_t)busRead(0xFFFD) << 8);

    addrAbs = addrRel = accOperation = 0;
    cycles = 7;
    cycleCount = 0;

    m_currentInstruction = lookup[0xEA];
}

void MOS6502::IRQ(){

    irqPending = true;
}

void MOS6502::irqHandler(){

    busWrite(stackPos + registers.sp, (uint8_t)((registers.pc & 0xFF00) >> 8));
    registers.sp--;
    busWrite(stackPos + registers.sp, (uint8_t)(registers.pc & 0xFF));
    registers.sp--;

    uint8_t status = registers.status.c;
    status |= registers.status.z << 1;
    status |= registers.status.i << 2;
    status |= registers.status.d << 3;
    status |= 0x0 << 4;
    status |= 0x1 << 5; //Status 5 always 1.
    status |= registers.status.v << 6;
    status |= registers.status.n << 7;
    busWrite(stackPos + registers.sp, status);
    registers.sp--;

    registers.pc = busRead(0xFFFE) | ((uint16_t)busRead(0xFFFF) << 8);

    registers.status.i = 1;

    cycles += 7;
}

void MOS6502::NMI(){
    nmiPending = true;
}

void MOS6502::nmiHandler(){

    busWrite(stackPos + registers.sp, (uint8_t)((registers.pc & 0xFF00) >> 8));
    registers.sp--;
    busWrite(stackPos + registers.sp, (uint8_t)(registers.pc & 0xFF));
    registers.sp--;

    uint8_t status = registers.status.c;
    status |= registers.status.z << 1;
    status |= registers.status.i << 2;
    status |= registers.status.d << 3;
    status |= 0x0 << 4;
    status |= 0x1 << 5; //Status 5 always 1.
    status |= registers.status.v << 6;
    status |= registers.status.n << 7;
    busWrite(stackPos + registers.sp, status);
    registers.sp--;

    registers.pc = busRead(0xFFFA) | ((uint16_t)busRead(0xFFFB) << 8);

    registers.status.i = 1;

    cycles += 7;
}

void MOS6502::clk(){

    if(cycles == 0){

        irqEligible = !registers.status.i;

        m_currentOpcode = busRead(registers.pc++);
        m_currentInstruction = lookup[m_currentOpcode];
        uint8_t addrRet = (this->*(m_currentInstruction.addrMode))();
        uint8_t instrRet = (this->*(m_currentInstruction.instrCode))();

        if(
            //opcode == 0x40  // RTI

                m_currentOpcode != 0x78 && // SEI
                m_currentOpcode != 0x58 && // CLI
                m_currentOpcode != 0x28    // PLP
                ){
            irqEligible = !registers.status.i;
        }

        if(nmiPending){

            if(m_currentOpcode == 0x00)
                registers.pc = busRead(0xFFFA) | ((uint16_t)busRead(0xFFFB) << 8);
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

MOS6502::registers_t* MOS6502::getRegisters(){
    return &registers;
}

const char * MOS6502::currentMnemonic() const{
    return m_currentInstruction.mnemonic;
}

std::string MOS6502::currentInstruction() const{

    return buildInstrString(m_currentInstruction);
}

std::string MOS6502::nextInstruction() const{

    uint8_t opcode = busRead(registers.pc);
    return buildInstrString(lookup[opcode]);
}

std::string MOS6502::nextOpcode() const{
    return lookup[busRead(registers.pc)].mnemonic;
}

uint64_t MOS6502::getCycleCount() const{
    return cycleCount;
}

uint8_t MOS6502::getRemainingCycles() const{
    return cycles;
}

std::vector<std::function<void(void)>> MOS6502::getGUIs() {

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

    addrAbs = registers.pc++;
    return 0;
}

uint8_t MOS6502::ABS(){

    addrAbs = busRead(registers.pc) | (busRead(registers.pc + 1) << 8);
    registers.pc += 2;
    return 0;
}

uint8_t MOS6502::ZP0(){

    addrAbs = busRead(registers.pc) & 0x00FF;
    registers.pc++;
    return 0;
}

uint8_t MOS6502::REL(){

    addrRel = busRead(registers.pc);
    registers.pc++;

    if(addrRel & 0x80) //If the byte was negative, make whole addr negative.
        addrRel |= 0xFF00;

    addrAbs = registers.pc + addrRel;

    return 0;
}

uint8_t MOS6502::ID0(){

    addrRel = busRead(registers.pc) | (busRead(registers.pc + 1) << 8);

    //HW bug implementation. If the lo byte of pointer is 0xFF,
    //CPU wraps back to the same page.
    if((addrRel & 0x00FF) == 0x00FF)
        addrAbs = busRead(addrRel) | (busRead(addrRel & 0xFF00) << 8);
    else
        addrAbs = busRead(addrRel) | (busRead(addrRel + 1) << 8);

    //pc increment is not needed, JMP will change pc anyways.
    return 0;
}

uint8_t MOS6502::ABX(){

    addrRel = busRead(registers.pc) | (busRead(registers.pc + 1) << 8);
    addrAbs = addrRel + registers.x;
    registers.pc += 2;

    if((addrRel & 0xFF00) == (addrAbs & 0xFF00))
        return 0;
    else
        return 1;
}

uint8_t MOS6502::ABY(){

    addrRel = busRead(registers.pc) | (busRead(registers.pc + 1) << 8);
    addrAbs = addrRel + registers.y;
    registers.pc += 2;

    if((addrRel & 0xFF00) == (addrAbs & 0xFF00))
        return 0;
    else
        return 1;
}

uint8_t MOS6502::ZPX(){

    addrRel = busRead(registers.pc);
    addrAbs = (addrRel + registers.x) & 0x00FF;
    registers.pc++;

    return 0;
}

uint8_t MOS6502::ZPY(){

    addrRel = busRead(registers.pc);
    addrAbs = (addrRel + registers.y) & 0x00FF;
    registers.pc++;

    return 0;
}

uint8_t MOS6502::IDX(){

    addrRel = (busRead(registers.pc) + registers.x);
    addrAbs = busRead(addrRel & 0x00FF) | (busRead((addrRel + 1) & 0x00FF) << 8);
    registers.pc++;

    return 0;
}

uint8_t MOS6502::IDY(){

    addrRel = busRead(registers.pc);
    uint16_t newAddr = (busRead(addrRel) | ((uint16_t)busRead((addrRel + 1) & 0x00FF) << 8));
    addrAbs = newAddr + registers.y;
    registers.pc++;

    if((addrAbs & 0xFF00) == (newAddr & 0xFF00))
        return 0;
    else
        return 1;
}

// =========================================================================================
//Instructions.

uint8_t MOS6502::ADC(){

    uint8_t memoryValue = busRead(addrAbs);

    bool memoryNegative = (memoryValue & 0x80) == 0x80;
    bool accNegative = (registers.acc & 0x80) == 0x80;

    uint16_t result = (uint16_t)registers.acc + (uint16_t)memoryValue + (uint16_t)registers.status.c;
    registers.status.c = (result & 0x100) == 0x100;
    result &= 0xFF;
    registers.status.z = result == 0x0;
    registers.status.n = (result & 0x80) == 0x80;

    if(memoryNegative != accNegative)
        registers.status.v = 0;
    else
        registers.status.v = memoryNegative == accNegative && memoryNegative != registers.status.n;

    registers.acc = result;

    return 1;
}

uint8_t MOS6502::AND(){

    registers.acc = registers.acc & busRead(addrAbs);
    registers.status.z = registers.acc == 0x0;
    registers.status.n = (registers.acc & 0x80) == 0x80;

    return 1;
}

uint8_t MOS6502::ASL(){

    if(accOperation){

        registers.status.c = (registers.acc & 0x80) >> 7;
        registers.acc <<= 1;
        registers.status.z = registers.acc == 0x0;
        registers.status.n = (registers.acc & 0x80) == 0x80;
        accOperation = false;
    } else {

        uint8_t value = busRead(addrAbs);
        registers.status.c = (value & 0x80) >> 7;
        value <<= 1;
        registers.status.z = value == 0x0;
        registers.status.n = (value & 0x80) == 0x80;
        busWrite(addrAbs, value);
    }

    return 0;
}

uint8_t MOS6502::BCC(){

    branch(!registers.status.c);
    return 0;
}

uint8_t MOS6502::BCS(){

    branch(registers.status.c);
    return 0;
}

uint8_t MOS6502::BEQ(){

    branch(registers.status.z);
    return 0;
}

uint8_t MOS6502::BIT(){

    uint8_t value = busRead(addrAbs);

    registers.status.n = (value & 0x80) >> 7;
    registers.status.v = (value & 0x40) >> 6;
    registers.status.z = (value & registers.acc) == 0x0;

    return 0;
}

uint8_t MOS6502::BMI(){

    branch(registers.status.n);
    return 0;
}

uint8_t MOS6502::BNE(){

    branch(!registers.status.z);
    return 0;
}

uint8_t MOS6502::BPL(){

    branch(!registers.status.n);
    return 0;
}

uint8_t MOS6502::BRK(){

    registers.pc++;

    busWrite(stackPos + registers.sp, (registers.pc & 0xFF00) >> 8);
    registers.sp--;
    busWrite(stackPos + registers.sp, registers.pc & 0xFF);
    registers.sp--;

    uint8_t status = registers.status.c;
    status |= registers.status.z << 1;
    status |= registers.status.i << 2;
    status |= registers.status.d << 3;
    status |= 0x1 << 4;
    status |= 0x1 << 5; //Status 5 always 1.
    status |= registers.status.v << 6;
    status |= registers.status.n << 7;
    busWrite(stackPos + registers.sp, status);
    registers.sp--;

    registers.status.i = 1;

    registers.pc = busRead(0xFFFE) | ((uint16_t)busRead(0xFFFF) << 8);

    return 0;
}

uint8_t MOS6502::BVC(){

    branch(!registers.status.v);
    return 0;
}

uint8_t MOS6502::BVS(){

    branch(registers.status.v);
    return 0;
}

uint8_t MOS6502::CLC(){

    registers.status.c = 0;
    return 0;
}

uint8_t MOS6502::CLD(){

    registers.status.d = 0;
    return 0;
}

uint8_t MOS6502::CLI(){

    registers.status.i = 0;
    return 0;
}

uint8_t MOS6502::CLV(){

    registers.status.v = 0;
    return 0;
}

uint8_t MOS6502::CMP(){

    uint8_t data = busRead(addrAbs);
    registers.status.c = (registers.acc >= data);
    registers.status.z = ((registers.acc & 0x00FF) == data);
    registers.status.n = (((registers.acc - data) & 0x80) == 0x80);
    return 1;
}

uint8_t MOS6502::CPX(){

    uint8_t data = busRead(addrAbs);
    registers.status.c = (registers.x >= data);
    registers.status.z = ((registers.x & 0x00FF) == data);
    registers.status.n = (((registers.x - data) & 0x80) == 0x80);
    return 1;
}

uint8_t MOS6502::CPY(){

    uint8_t data = busRead(addrAbs);
    registers.status.c = (registers.y >= data);
    registers.status.z = (registers.y == data);
    registers.status.n = (((registers.y - data) & 0x80) == 0x80);
    return 1;
}

uint8_t MOS6502::DEC(){

    uint8_t newVal = busRead(addrAbs) - 1;
    busWrite(addrAbs, newVal);

    registers.status.z = newVal == 0;
    registers.status.n = (newVal & 0x80) == 0x80;

    return 0;
}

uint8_t MOS6502::DEX(){

    registers.x--;

    registers.status.z = registers.x == 0;
    registers.status.n = (registers.x & 0x80) == 0x80;

    return 0;
}

uint8_t MOS6502::DEY(){

    uint8_t newVal = --registers.y;

    registers.status.z = newVal == 0;
    registers.status.n = (newVal & 0x80) == 0x80;

    return 0;
}

uint8_t MOS6502::EOR(){

    registers.acc = busRead(addrAbs) ^ registers.acc;

    registers.status.z = registers.acc == 0;
    registers.status.n = (registers.acc & 0x80) == 0x80;
    return 1;
}

uint8_t MOS6502::INC(){

    uint8_t newVal = busRead(addrAbs) + 1;
    busWrite(addrAbs, newVal);

    registers.status.z = newVal == 0;
    registers.status.n = (newVal & 0x80) == 0x80;
    return 0;
}

uint8_t MOS6502::INX(){

    registers.x++;

    registers.status.z = registers.x == 0;
    registers.status.n = (registers.x & 0x80) == 0x80;

    return 0;
}

uint8_t MOS6502::INY(){

    registers.y++;

    registers.status.z = registers.y == 0;
    registers.status.n = (registers.y & 0x80) == 0x80;

    return 0;
}

uint8_t MOS6502::JMP(){

    registers.pc = addrAbs;
    return 0;
}

uint8_t MOS6502::JSR(){

    registers.pc--;

    busWrite(stackPos + registers.sp, (registers.pc >> 8) & 0x00FF);
    registers.sp--;
    busWrite(stackPos + registers.sp, registers.pc & 0x00FF);
    registers.sp--;

    registers.pc = addrAbs;
    return 0;
}

uint8_t MOS6502::LDA(){

    registers.acc = busRead(addrAbs);

    registers.status.z = registers.acc == 0;
    registers.status.n = (registers.acc & 0x80) == 0x80;

    return 1;
}

uint8_t MOS6502::LDX(){

    registers.x = busRead(addrAbs);

    registers.status.z = registers.x == 0;
    registers.status.n = (registers.x & 0x80) == 0x80;

    return 1;
}

uint8_t MOS6502::LDY(){

    registers.y = busRead(addrAbs);

    registers.status.z = registers.y == 0;
    registers.status.n = (registers.y & 0x80) == 0x80;

    return 1;
}

uint8_t MOS6502::LSR(){

    if(accOperation){
        registers.status.c = registers.acc & 0x1;
        registers.acc >>= 1;
        registers.status.z = registers.acc == 0;
        registers.status.n = (registers.acc & 0x80) == 0x80;
        accOperation = false;
    } else {
        registers.status.c = busRead(addrAbs) & 0x1;
        uint8_t newVal = busRead(addrAbs) >> 1;
        busWrite(addrAbs, newVal);
        registers.status.z = newVal == 0;
        registers.status.n = (newVal & 0x80) == 0x80;
    }

    return 0;
}

uint8_t MOS6502::NOP(){

    return 1;
}

uint8_t MOS6502::ORA(){

    registers.acc |= busRead(addrAbs);
    registers.status.z = registers.acc == 0;
    registers.status.n = (registers.acc & 0x80) == 0x80;

    return 1;
}

uint8_t MOS6502::PHA(){

    busWrite(stackPos + registers.sp, registers.acc);
    registers.sp--;
    return 0;
}

uint8_t MOS6502::PHP(){

    uint8_t status = registers.status.c;
    status |= registers.status.z << 1;
    status |= registers.status.i << 2;
    status |= registers.status.d << 3;
    status |= 0x1 << 4;
    status |= 0x1 << 5; //Status 5 always 1.
    status |= registers.status.v << 6;
    status |= registers.status.n << 7;

    busWrite(stackPos + registers.sp, status);
    registers.sp--;
    return 0;
}

uint8_t MOS6502::PLA(){

    registers.sp++;
    registers.acc = busRead(stackPos + registers.sp);
    registers.status.z = registers.acc == 0;
    registers.status.n = (registers.acc & 0x80) == 0x80;

    return 0;
}

uint8_t MOS6502::PLP(){

    registers.sp++;
    uint8_t status = busRead(stackPos + registers.sp);
    registers.status.c = status & 0x1;
    registers.status.z = (status & 0x2) >> 1;
    registers.status.i = (status & 0x4) >> 2;
    registers.status.d = (status & 0x8) >> 3;
    registers.status.b = (status & 0x10) >> 4;
    registers.status.v = (status & 0x40) >> 6;
    registers.status.n = (status & 0x80) >> 7;

    return 0;
}

uint8_t MOS6502::ROL(){

    uint8_t oldCarry = registers.status.c;
    // Accumulator operation.
    if(accOperation){

        registers.status.c = (registers.acc & 0x80) >> 7;
        registers.acc <<= 1;
        registers.acc |= oldCarry;
        registers.status.n = (registers.acc & 0x80) == 0x80;
        registers.status.z = registers.acc == 0x0;

        accOperation = false;

        // Memory operation.
    } else {

        uint8_t value = busRead(addrAbs);

        registers.status.c = (value & 0x80) >> 7;
        value <<= 1;
        value |= oldCarry;
        registers.status.n = (value & 0x80) == 0x80;
        registers.status.z = value == 0x0;

        busWrite(addrAbs, value);
    }

    return 0;
}

uint8_t MOS6502::ROR(){

    uint8_t oldCarry = registers.status.c << 7;
    // Accumulator operation.
    if(accOperation){

        registers.status.c = (registers.acc & 0x1);
        registers.acc >>= 1;
        registers.acc |= oldCarry;
        registers.status.n = (registers.acc & 0x80) == 0x80;
        registers.status.z = registers.acc == 0x0;

        accOperation = false;
        // Memory operation.
    } else {

        uint8_t value = busRead(addrAbs);

        registers.status.c = (value & 0x1);
        value >>= 1;
        value |= oldCarry;
        registers.status.n = (value & 0x80) == 0x80;
        registers.status.z = value == 0x0;

        busWrite(addrAbs, value);
    }

    return 0;
}

uint8_t MOS6502::RTI(){

    registers.sp++;
    uint8_t flags = busRead(stackPos + registers.sp);
    registers.sp++;
    registers.pc = busRead(stackPos + registers.sp) | (busRead(stackPos + registers.sp + 1) << 8);
    registers.sp++;

    registers.status.c = flags & 0x1;
    registers.status.z = (flags & 0x2) >> 1;
    registers.status.i = (flags & 0x4) >> 2;
    registers.status.d = (flags & 0x8) >> 3;
    registers.status.b = 0;
    registers.status.x = 0;
    registers.status.v = (flags & 0x40) >> 6;
    registers.status.n = (flags & 0x80) >> 7;

    return 0;
}

uint8_t MOS6502::RTS(){

    registers.sp++;
    registers.pc = ((busRead(stackPos + registers.sp) | ((uint16_t)busRead(stackPos + registers.sp + 1) << 8))) + 1;
    registers.sp++;

    return 0;
}

uint8_t MOS6502::SBC(){

    uint16_t memoryValue = (uint8_t)(~busRead(addrAbs));

    uint16_t result = (uint16_t)registers.acc + memoryValue + (uint16_t)(registers.status.c);
    registers.status.c = result & 0xFF00;
    result &= 0xFF;
    registers.status.n = (result & 0x80) == 0x80;
    registers.status.z = result == 0x0;
    registers.status.v = (result ^ (uint16_t)registers.acc) & (result ^ memoryValue) & 0x0080;

    registers.acc = result;

    return 1;
}

uint8_t MOS6502::SEC(){

    registers.status.c = 1;
    return 0;
}

uint8_t MOS6502::SED(){

    registers.status.d = 1;
    return 0;
}

uint8_t MOS6502::SEI(){

    registers.status.i = 1;
    return 0;
}

uint8_t MOS6502::STA(){

    busWrite(addrAbs, registers.acc);
    return 0;
}

uint8_t MOS6502::STX(){

    busWrite(addrAbs, registers.x);
    return 0;
}

uint8_t MOS6502::STY(){

    busWrite(addrAbs, registers.y);
    return 0;
}

uint8_t MOS6502::TAX(){

    registers.x = registers.acc;
    registers.status.z = registers.x == 0x0;
    registers.status.n = (registers.x & 0x80) == 0x80;
    return 0;
}

uint8_t MOS6502::TAY(){

    registers.y = registers.acc;
    registers.status.z = registers.y == 0x0;
    registers.status.n = (registers.y & 0x80) == 0x80;
    return 0;
}

uint8_t MOS6502::TSX(){

    registers.x = registers.sp;
    registers.status.z = registers.x == 0x0;
    registers.status.n = (registers.x & 0x80) == 0x80;
    return 0;
}

uint8_t MOS6502::TXA(){

    registers.acc = registers.x;
    registers.status.z = registers.acc == 0x0;
    registers.status.n = (registers.acc & 0x80) == 0x80;
    return 0;
}

uint8_t MOS6502::TXS(){

    registers.sp = registers.x;
    return 0;
}

uint8_t MOS6502::TYA(){

    registers.acc = registers.y;
    registers.status.z = registers.acc == 0x0;
    registers.status.n = (registers.acc & 0x80) == 0x80;
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
    registers.status.c = (busRead(addrAbs) & 0x80) >> 7;

    return 0;
}

uint8_t MOS6502::ANE(){

    registers.acc |= 0xFF;
    registers.acc &= registers.x;
    registers.acc &= busRead(addrAbs);

    registers.status.n = (registers.acc & 0x80) == 0x80;
    registers.status.z = registers.acc == 0x0;

    return 0;
}

uint8_t MOS6502::ARR(){

    AND();
    ACC(); // Because ROR is performed on A.
    ROR();
    registers.status.c = (registers.acc & 0x40) >> 6;
    registers.status.v = ((registers.acc & 0x40) >> 6) ^ ((registers.acc & 0x20) >> 5);

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

    uint8_t value = busRead(addrAbs) & registers.sp;
    registers.acc = value;
    registers.x = value;
    registers.sp = value;

    registers.status.n = (value & 0x80) == 0x80;
    registers.status.z = value == 0x0;

    return 1;
}

uint8_t MOS6502::LAX(){

    LDA();
    LDX();

    return 1;
}

uint8_t MOS6502::LXA(){

    registers.acc |= 0xFF;
    registers.acc &= busRead(addrAbs);
    registers.x = registers.acc;

    registers.status.n = (registers.acc & 0x80) == 0x80;
    registers.status.z = registers.acc == 0x0;

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

    busWrite(addrAbs, registers.acc & registers.x);
    return 0;
}

uint8_t MOS6502::SBX(){

    registers.x = (registers.acc & registers.x) - busRead(addrAbs);
    registers.status.n = (registers.x & 0x80) == 0x80;
    registers.status.z = registers.x == 0x0;
    registers.status.c = (registers.acc >= registers.x);
    return 0;
}

uint8_t MOS6502::SHA(){

    busWrite(addrAbs, registers.acc & registers.x & ((addrAbs & 0xFF00 >> 8) + 1));
    return 0;
}

uint8_t MOS6502::SHX(){

    busWrite(addrAbs, registers.x & (((addrRel & 0xFF00) >> 8) + 1));
    return 0;
}

uint8_t MOS6502::SHY(){

    busWrite(addrAbs, registers.y & (((addrRel & 0xFF00) >> 8) + 1));
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

    registers.sp = registers.acc & registers.x;
    busWrite(addrAbs, registers.acc & registers.x & (((addrAbs & 0xFF00) >> 8) + 1));

    return 0;
}

uint8_t MOS6502::JAM(){

    registers.pc--; //Decreases the PC to the itself, effectively stopping the execution process.
    return 0;
}

void MOS6502::init() {
    hardReset();
}


