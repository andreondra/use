//
// Created by golas on 21.2.23.
//

#include "components/6502.h"
#include "Connector.h"
#include "imgui.h"
#include <memory>
#include <sstream>

MOS6502::MOS6502() : m_currentInstruction(lookup[0xEA]) {

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
        .set = [this](bool active) {
            IRQ(active);
        }
    });

    m_ports["mainBus"] = &m_mainBus;
    m_deviceName = "6502 CPU";

    hardReset();
}

MOS6502::~MOS6502(){}

void MOS6502::branch(bool condition){

    if(condition){

        m_cycles++;

        if((m_addrAbs & 0xFF00) != (m_registers.pc & 0xFF00))
            m_cycles++;

        m_registers.pc = m_addrAbs; // Do not move above the 'if', old PC value is checked.
    }
}

std::string MOS6502::getCurrentAddressString() {

    std::stringstream instrStr;

    if(m_currentInstruction.addrMode == &MOS6502::ACC)
        instrStr << "A";
    else if(m_currentInstruction.addrMode == &MOS6502::IMM)
        instrStr << "#$" << std::hex << (int)m_mainBus.read(m_registers.pc - 1) << " (IMM)";
    else if(m_currentInstruction.addrMode == &MOS6502::ABS)
        instrStr << "$" << std::hex << m_addrAbs << " (ABS)";
    else if(m_currentInstruction.addrMode == &MOS6502::ZP0)
        instrStr << "$" << std::hex << m_addrAbs << " (ZP0)";
    else if(m_currentInstruction.addrMode == &MOS6502::REL)
        instrStr << "$" << std::hex << m_addrRel << " (REL)";
    else if(m_currentInstruction.addrMode == &MOS6502::ID0)
        instrStr << "($" << std::hex << m_addrAbs << ")" << " (ID0)";
    else if(m_currentInstruction.addrMode == &MOS6502::ABX)
        instrStr << "$" << std::hex << m_addrAbs << ",X" << " (ABX)";
    else if(m_currentInstruction.addrMode == &MOS6502::ABY)
        instrStr << "$" << std::hex << m_addrAbs << ",Y" << " (ABY)";
    else if(m_currentInstruction.addrMode == &MOS6502::ZPX)
        instrStr << "$" << std::hex << m_addrAbs << ",X" << " (ZPX)";
    else if(m_currentInstruction.addrMode == &MOS6502::ZPY)
        instrStr << "$" << std::hex << m_addrAbs << ",Y" << " (ZPY)";
    else if(m_currentInstruction.addrMode == &MOS6502::IDX)
        instrStr << "($" << std::hex << m_addrAbs << ",X)" << " (IDX)";
    else if(m_currentInstruction.addrMode == &MOS6502::IDY)
        instrStr << "($" << std::hex << m_addrAbs << "),Y" << " (IDY)";
    else if(m_currentInstruction.addrMode == &MOS6502::IMP)
        instrStr << "(IMP)";
    else
        instrStr << "none";

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
    m_registers.pc = m_mainBus.read(VECTOR_RST) | ((uint16_t)m_mainBus.read(VECTOR_RST + 1) << 8);

    m_addrAbs = m_addrRel = 0;
    m_accOperation = false;
    m_cycles = 7;
    m_cycleCount = 0;

    m_nmi = false;
    m_nmiPending = false;
    m_irq = false;
    m_irqPending = false;

    m_currentOpcode = 0xEA;
    m_currentInstruction = lookup[m_currentOpcode];
}

void MOS6502::softReset(){

    m_registers.sp -= 3;
    m_registers.status.i = 1;
    m_mainBus.write(0x4015, 0x00);

    m_registers.pc = m_mainBus.read(0xFFFC) | ((uint16_t)m_mainBus.read(0xFFFD) << 8);

    m_addrAbs = m_addrRel = 0;
    m_accOperation = false;
    m_cycles = 7;
    m_cycleCount = 0;

    m_currentInstruction = lookup[0xEA];
}

void MOS6502::IRQ(bool active){
    m_irq = active;
}

void MOS6502::irqHandler(){

    m_mainBus.write(STACK_POSITION + m_registers.sp, (uint8_t)((m_registers.pc & 0xFF00) >> 8));
    m_registers.sp--;
    m_mainBus.write(STACK_POSITION + m_registers.sp, (uint8_t)(m_registers.pc & 0xFF));
    m_registers.sp--;

    uint8_t status = m_registers.status.c;
    status |= m_registers.status.z << 1;
    status |= m_registers.status.i << 2;
    status |= m_registers.status.d << 3;
    status |= 0x0 << 4;
    status |= 0x1 << 5; //Status 5 always 1.
    status |= m_registers.status.v << 6;
    status |= m_registers.status.n << 7;
    m_mainBus.write(STACK_POSITION + m_registers.sp, status);
    m_registers.sp--;

    m_registers.pc = m_mainBus.read(VECTOR_IRQ) | ((uint16_t)m_mainBus.read(VECTOR_IRQ + 1) << 8);

    m_registers.status.i = 1;

    m_cycles += 7;
}

void MOS6502::NMI(){
    m_nmi = true;
}

void MOS6502::nmiHandler(){

    m_mainBus.write(STACK_POSITION + m_registers.sp, (uint8_t)((m_registers.pc & 0xFF00) >> 8));
    m_registers.sp--;
    m_mainBus.write(STACK_POSITION + m_registers.sp, (uint8_t)(m_registers.pc & 0xFF));
    m_registers.sp--;

    uint8_t status = m_registers.status.c;
    status |= m_registers.status.z << 1;
    status |= m_registers.status.i << 2;
    status |= m_registers.status.d << 3;
    status |= 0x0 << 4;
    status |= 0x1 << 5; //Status 5 always 1.
    status |= m_registers.status.v << 6;
    status |= m_registers.status.n << 7;
    m_mainBus.write(STACK_POSITION + m_registers.sp, status);
    m_registers.sp--;

    m_registers.pc = m_mainBus.read(VECTOR_NMI) | ((uint16_t)m_mainBus.read(VECTOR_NMI + 1) << 8);

    m_registers.status.i = 1;

    m_cycles += 7;
}

void MOS6502::CLK(){

    // ---------------------------------------------------------------
    // During the final clock of the instruction the interrupt state is checked.
    // If the interrupt is pending, the ISR is executed instead of the
    // next instruction.
    if(m_cycles == 1) {

        // NMI has the highest priority. IRQ is cancelled.
        if (m_nmiPending) {

            m_next = nextMode_t::NMI_ISR;
            m_irqPending = m_nmiPending = false;

        // IRQ has the second-highest priority.
        } else if (m_irqPending) {

            bool interruptMask;

            // All two-cycle instruction poll interrupts after changing the flags,
            // so we need to check the old value.
            if (
                    m_currentOpcode == 0x78 || // SEI
                    m_currentOpcode == 0x58 || // CLI
                    m_currentOpcode == 0x28    // PLP
                    ) {
                interruptMask = m_oldInterruptMask;
            } else {
                interruptMask = m_registers.status.i;
            }

            if (!interruptMask) m_next = nextMode_t::IRQ_ISR;
            else                m_next = nextMode_t::INSTRUCTION;

            m_irqPending = false;

        // No interrupt, next instruction will be executed.
        } else {
            m_next = nextMode_t::INSTRUCTION;
        }
    }
    // ---------------------------------------------------------------
    // When no clocks are remaining, fetch and execute the next instruction
    // or the ISR.
    if(m_cycles == 0){

        // Decide upon what will be executed next.
        // The most important is the value of the program counter register.
        // The handlers themselves do not poll for interrupts, so at least
        // one instruction is always executed before next interrupt.
        switch(m_next) {

            case nextMode_t::INSTRUCTION:
                // Nothing to do. Use current PC value.
                break;
            case nextMode_t::NMI_ISR:
                // Backup PC + status and get a new PC from vector at 0xFFFA.
                nmiHandler();
                break;
            case nextMode_t::IRQ_ISR:
                // Backup PC + status and get a new PC from vector at 0xFFFE.
                irqHandler();
                break;
        }

        m_oldInterruptMask      = m_registers.status.i;
        m_currentOpcode         = m_mainBus.read(m_registers.pc++);
        m_currentInstruction    = lookup[m_currentOpcode];
        uint8_t addrRet         = (this->*(m_currentInstruction.addrMode))();
        uint8_t instrRet        = (this->*(m_currentInstruction.instrCode))();
        m_cycles                  += m_currentInstruction.cycles;
        if (addrRet && instrRet) m_cycles++;
    }

    // ---------------------------------------------------------------
    // A state of the interrupt pins is checked on every clock.
    // (The current detected state is valid during the next clock.)
    // The state is checked by the edge/level detectors and sent to
    // an internal signal for further processing.
    if(m_nmi) {
        m_nmiPending = true;
        m_nmi = false;
    }

    if(m_irq) {
        m_irqPending = true;
    }
    // ---------------------------------------------------------------

    m_cycles--;
    m_cycleCount++;
}

bool MOS6502::instrFinished() const {
    return m_cycles == 0;
}

std::vector<EmulatorWindow> MOS6502::getGUIs() {

    std::function<void(void)> debugger = [this](){

        // Window contents
        // ===================================================================
        ImGui::SeparatorText("Current instruction");
        ImGui::Text("Mnemonic: %s", m_currentInstruction.mnemonic);
        ImGui::Text("Cycles: %u/%u", m_cycles, m_currentInstruction.cycles);
        ImGui::Text("Size: %u B", m_currentInstruction.instrLen);
        ImGui::Text("Address mode: %s", getCurrentAddressString().c_str());
        ImGui::Text("Remaining cycles: %u", m_cycles);

        ImGui::SeparatorText("Registers");
        ImGui::InputScalar("PC", ImGuiDataType_U16, &m_registers.pc, nullptr, nullptr, "%x", ImGuiInputTextFlags_CharsHexadecimal);
        ImGui::InputScalar("SP", ImGuiDataType_U8, &m_registers.sp, nullptr, nullptr, "%x", ImGuiInputTextFlags_CharsHexadecimal);
        ImGui::InputScalar("ACC", ImGuiDataType_U8, &m_registers.acc, nullptr, nullptr, "%x", ImGuiInputTextFlags_CharsHexadecimal);
        ImGui::InputScalar("X", ImGuiDataType_U8, &m_registers.x, nullptr, nullptr, "%x", ImGuiInputTextFlags_CharsHexadecimal);
        ImGui::InputScalar("Y", ImGuiDataType_U8, &m_registers.y, nullptr, nullptr, "%x", ImGuiInputTextFlags_CharsHexadecimal);

        ImGui::SeparatorText("Status flags");
        ImGui::Checkbox("C", &m_registers.status.c);
        ImGui::SameLine();
        ImGui::Checkbox("Z", &m_registers.status.z);
        ImGui::SameLine();
        ImGui::Checkbox("I", &m_registers.status.i);
        ImGui::SameLine();
        ImGui::Checkbox("D", &m_registers.status.d);
        ImGui::Checkbox("B", &m_registers.status.b);
        ImGui::SameLine();
        ImGui::Checkbox("X", &m_registers.status.x);
        ImGui::SameLine();
        ImGui::Checkbox("V", &m_registers.status.v);
        ImGui::SameLine();
        ImGui::Checkbox("N", &m_registers.status.n);


        ImGui::SeparatorText("Interrupt vectors");
        ImGui::Text("NMI at: 0x%x", VECTOR_NMI);
        ImGui::Text("RESET at: 0x%x", VECTOR_RST);
        ImGui::Text("IRQ/BRK at: 0x%x", VECTOR_IRQ);
        ImGui::SeparatorText("Interrupt status");
        ImGui::BeginDisabled();
        ImGui::Checkbox("NMI signal active", &m_nmi);
        ImGui::Checkbox("NMI pending", &m_nmiPending);
        ImGui::Checkbox("IRQ signal active", &m_nmi);
        ImGui::Checkbox("IRQ pending", &m_nmiPending);
        ImGui::EndDisabled();

        ImGui::SeparatorText("Stack");
        ImGui::Text("Stack position: 0x%x", STACK_POSITION);
        ImGui::SeparatorText("Stats");
        ImGui::Text("All cycles: %llu", m_cycleCount);
    };

    return {
            EmulatorWindow{
                    .category = m_deviceName,
                    .title = "Debugger",
                    .id    = getDeviceID(),
                    .dock  = DockSpace::LEFT,
                    .guiFunction = debugger
            }
    };
}
// =========================================================================================
//Addressing modes.
uint8_t MOS6502::ACC(){
    m_accOperation = true;
    return 0;
}

uint8_t MOS6502::IMP(){

    return 0;
}

uint8_t MOS6502::IMM(){

    m_addrAbs = m_registers.pc++;
    return 0;
}

uint8_t MOS6502::ABS(){

    m_addrAbs = m_mainBus.read(m_registers.pc) | (m_mainBus.read(m_registers.pc + 1) << 8);
    m_registers.pc += 2;
    return 0;
}

uint8_t MOS6502::ZP0(){

    m_addrAbs = m_mainBus.read(m_registers.pc) & 0x00FF;
    m_registers.pc++;
    return 0;
}

uint8_t MOS6502::REL(){

    m_addrRel = m_mainBus.read(m_registers.pc);
    m_registers.pc++;

    if(m_addrRel & 0x80) //If the byte was negative, make whole addr negative.
        m_addrRel |= 0xFF00;

    m_addrAbs = m_registers.pc + m_addrRel;

    return 0;
}

uint8_t MOS6502::ID0(){

    m_addrRel = m_mainBus.read(m_registers.pc) | (m_mainBus.read(m_registers.pc + 1) << 8);

    //HW bug implementation. If the lo byte of pointer is 0xFF,
    //CPU wraps back to the same page.
    if((m_addrRel & 0x00FF) == 0x00FF)
        m_addrAbs = m_mainBus.read(m_addrRel) | (m_mainBus.read(m_addrRel & 0xFF00) << 8);
    else
        m_addrAbs = m_mainBus.read(m_addrRel) | (m_mainBus.read(m_addrRel + 1) << 8);

    //pc increment is not needed, JMP will change pc anyways.
    return 0;
}

uint8_t MOS6502::ABX(){

    m_addrRel = m_mainBus.read(m_registers.pc) | (m_mainBus.read(m_registers.pc + 1) << 8);
    m_addrAbs = m_addrRel + m_registers.x;
    m_registers.pc += 2;

    if((m_addrRel & 0xFF00) == (m_addrAbs & 0xFF00))
        return 0;
    else
        return 1;
}

uint8_t MOS6502::ABY(){

    m_addrRel = m_mainBus.read(m_registers.pc) | (m_mainBus.read(m_registers.pc + 1) << 8);
    m_addrAbs = m_addrRel + m_registers.y;
    m_registers.pc += 2;

    if((m_addrRel & 0xFF00) == (m_addrAbs & 0xFF00))
        return 0;
    else
        return 1;
}

uint8_t MOS6502::ZPX(){

    m_addrRel = m_mainBus.read(m_registers.pc);
    m_addrAbs = (m_addrRel + m_registers.x) & 0x00FF;
    m_registers.pc++;

    return 0;
}

uint8_t MOS6502::ZPY(){

    m_addrRel = m_mainBus.read(m_registers.pc);
    m_addrAbs = (m_addrRel + m_registers.y) & 0x00FF;
    m_registers.pc++;

    return 0;
}

uint8_t MOS6502::IDX(){

    m_addrRel = (m_mainBus.read(m_registers.pc) + m_registers.x);
    m_addrAbs = m_mainBus.read(m_addrRel & 0x00FF) | (m_mainBus.read((m_addrRel + 1) & 0x00FF) << 8);
    m_registers.pc++;

    return 0;
}

uint8_t MOS6502::IDY(){

    m_addrRel = m_mainBus.read(m_registers.pc);
    uint16_t newAddr = (m_mainBus.read(m_addrRel) | ((uint16_t)m_mainBus.read((m_addrRel + 1) & 0x00FF) << 8));
    m_addrAbs = newAddr + m_registers.y;
    m_registers.pc++;

    if((m_addrAbs & 0xFF00) == (newAddr & 0xFF00))
        return 0;
    else
        return 1;
}

// =========================================================================================
//Instructions.

uint8_t MOS6502::ADC(){

    uint8_t memoryValue = m_mainBus.read(m_addrAbs);

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

    m_registers.acc = m_registers.acc & m_mainBus.read(m_addrAbs);
    m_registers.status.z = m_registers.acc == 0x0;
    m_registers.status.n = (m_registers.acc & 0x80) == 0x80;

    return 1;
}

uint8_t MOS6502::ASL(){

    if(m_accOperation){

        m_registers.status.c = (m_registers.acc & 0x80) >> 7;
        m_registers.acc <<= 1;
        m_registers.status.z = m_registers.acc == 0x0;
        m_registers.status.n = (m_registers.acc & 0x80) == 0x80;
        m_accOperation = false;
    } else {

        uint8_t value = m_mainBus.read(m_addrAbs);
        m_registers.status.c = (value & 0x80) >> 7;
        value <<= 1;
        m_registers.status.z = value == 0x0;
        m_registers.status.n = (value & 0x80) == 0x80;
        m_mainBus.write(m_addrAbs, value);
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

    uint8_t value = m_mainBus.read(m_addrAbs);

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

    m_mainBus.write(STACK_POSITION + m_registers.sp, (m_registers.pc & 0xFF00) >> 8);
    m_registers.sp--;
    m_mainBus.write(STACK_POSITION + m_registers.sp, m_registers.pc & 0xFF);
    m_registers.sp--;

    uint8_t status = m_registers.status.c;
    status |= m_registers.status.z << 1;
    status |= m_registers.status.i << 2;
    status |= m_registers.status.d << 3;
    status |= 0x1 << 4;
    status |= 0x1 << 5; //Status 5 always 1.
    status |= m_registers.status.v << 6;
    status |= m_registers.status.n << 7;
    m_mainBus.write(STACK_POSITION + m_registers.sp, status);
    m_registers.sp--;

    m_registers.status.i = 1;

    m_registers.pc = m_mainBus.read(VECTOR_IRQ) | ((uint16_t)m_mainBus.read(VECTOR_IRQ + 1) << 8);

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

    uint8_t data = m_mainBus.read(m_addrAbs);
    m_registers.status.c = (m_registers.acc >= data);
    m_registers.status.z = ((m_registers.acc & 0x00FF) == data);
    m_registers.status.n = (((m_registers.acc - data) & 0x80) == 0x80);
    return 1;
}

uint8_t MOS6502::CPX(){

    uint8_t data = m_mainBus.read(m_addrAbs);
    m_registers.status.c = (m_registers.x >= data);
    m_registers.status.z = ((m_registers.x & 0x00FF) == data);
    m_registers.status.n = (((m_registers.x - data) & 0x80) == 0x80);
    return 1;
}

uint8_t MOS6502::CPY(){

    uint8_t data = m_mainBus.read(m_addrAbs);
    m_registers.status.c = (m_registers.y >= data);
    m_registers.status.z = (m_registers.y == data);
    m_registers.status.n = (((m_registers.y - data) & 0x80) == 0x80);
    return 1;
}

uint8_t MOS6502::DEC(){

    uint8_t newVal = m_mainBus.read(m_addrAbs) - 1;
    m_mainBus.write(m_addrAbs, newVal);

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

    m_registers.acc = m_mainBus.read(m_addrAbs) ^ m_registers.acc;

    m_registers.status.z = m_registers.acc == 0;
    m_registers.status.n = (m_registers.acc & 0x80) == 0x80;
    return 1;
}

uint8_t MOS6502::INC(){

    uint8_t newVal = m_mainBus.read(m_addrAbs) + 1;
    m_mainBus.write(m_addrAbs, newVal);

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

    m_registers.pc = m_addrAbs;
    return 0;
}

uint8_t MOS6502::JSR(){

    m_registers.pc--;

    m_mainBus.write(STACK_POSITION + m_registers.sp, (m_registers.pc >> 8) & 0x00FF);
    m_registers.sp--;
    m_mainBus.write(STACK_POSITION + m_registers.sp, m_registers.pc & 0x00FF);
    m_registers.sp--;

    m_registers.pc = m_addrAbs;
    return 0;
}

uint8_t MOS6502::LDA(){

    m_registers.acc = m_mainBus.read(m_addrAbs);

    m_registers.status.z = m_registers.acc == 0;
    m_registers.status.n = (m_registers.acc & 0x80) == 0x80;

    return 1;
}

uint8_t MOS6502::LDX(){

    m_registers.x = m_mainBus.read(m_addrAbs);

    m_registers.status.z = m_registers.x == 0;
    m_registers.status.n = (m_registers.x & 0x80) == 0x80;

    return 1;
}

uint8_t MOS6502::LDY(){

    m_registers.y = m_mainBus.read(m_addrAbs);

    m_registers.status.z = m_registers.y == 0;
    m_registers.status.n = (m_registers.y & 0x80) == 0x80;

    return 1;
}

uint8_t MOS6502::LSR(){

    if(m_accOperation){
        m_registers.status.c = m_registers.acc & 0x1;
        m_registers.acc >>= 1;
        m_registers.status.z = m_registers.acc == 0;
        m_registers.status.n = (m_registers.acc & 0x80) == 0x80;
        m_accOperation = false;
    } else {
        m_registers.status.c = m_mainBus.read(m_addrAbs) & 0x1;
        uint8_t newVal = m_mainBus.read(m_addrAbs) >> 1;
        m_mainBus.write(m_addrAbs, newVal);
        m_registers.status.z = newVal == 0;
        m_registers.status.n = (newVal & 0x80) == 0x80;
    }

    return 0;
}

uint8_t MOS6502::NOP(){

    return 1;
}

uint8_t MOS6502::ORA(){

    m_registers.acc |= m_mainBus.read(m_addrAbs);
    m_registers.status.z = m_registers.acc == 0;
    m_registers.status.n = (m_registers.acc & 0x80) == 0x80;

    return 1;
}

uint8_t MOS6502::PHA(){

    m_mainBus.write(STACK_POSITION + m_registers.sp, m_registers.acc);
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

    m_mainBus.write(STACK_POSITION + m_registers.sp, status);
    m_registers.sp--;
    return 0;
}

uint8_t MOS6502::PLA(){

    m_registers.sp++;
    m_registers.acc = m_mainBus.read(STACK_POSITION + m_registers.sp);
    m_registers.status.z = m_registers.acc == 0;
    m_registers.status.n = (m_registers.acc & 0x80) == 0x80;

    return 0;
}

uint8_t MOS6502::PLP(){

    m_registers.sp++;
    uint8_t status = m_mainBus.read(STACK_POSITION + m_registers.sp);
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
    if(m_accOperation){

        m_registers.status.c = (m_registers.acc & 0x80) >> 7;
        m_registers.acc <<= 1;
        m_registers.acc |= oldCarry;
        m_registers.status.n = (m_registers.acc & 0x80) == 0x80;
        m_registers.status.z = m_registers.acc == 0x0;

        m_accOperation = false;

        // Memory operation.
    } else {

        uint8_t value = m_mainBus.read(m_addrAbs);

        m_registers.status.c = (value & 0x80) >> 7;
        value <<= 1;
        value |= oldCarry;
        m_registers.status.n = (value & 0x80) == 0x80;
        m_registers.status.z = value == 0x0;

        m_mainBus.write(m_addrAbs, value);
    }

    return 0;
}

uint8_t MOS6502::ROR(){

    uint8_t oldCarry = m_registers.status.c << 7;
    // Accumulator operation.
    if(m_accOperation){

        m_registers.status.c = (m_registers.acc & 0x1);
        m_registers.acc >>= 1;
        m_registers.acc |= oldCarry;
        m_registers.status.n = (m_registers.acc & 0x80) == 0x80;
        m_registers.status.z = m_registers.acc == 0x0;

        m_accOperation = false;
        // Memory operation.
    } else {

        uint8_t value = m_mainBus.read(m_addrAbs);

        m_registers.status.c = (value & 0x1);
        value >>= 1;
        value |= oldCarry;
        m_registers.status.n = (value & 0x80) == 0x80;
        m_registers.status.z = value == 0x0;

        m_mainBus.write(m_addrAbs, value);
    }

    return 0;
}

uint8_t MOS6502::RTI(){

    m_registers.sp++;
    uint8_t flags = m_mainBus.read(STACK_POSITION + m_registers.sp);
    m_registers.sp++;
    m_registers.pc = m_mainBus.read(STACK_POSITION + m_registers.sp) | (m_mainBus.read(STACK_POSITION + m_registers.sp + 1) << 8);
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
    m_registers.pc = ((m_mainBus.read(STACK_POSITION + m_registers.sp) | ((uint16_t)m_mainBus.read(STACK_POSITION + m_registers.sp + 1) << 8))) + 1;
    m_registers.sp++;

    return 0;
}

uint8_t MOS6502::SBC(){

    uint16_t memoryValue = (uint8_t)(~m_mainBus.read(m_addrAbs));

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

    m_mainBus.write(m_addrAbs, m_registers.acc);
    return 0;
}

uint8_t MOS6502::STX(){

    m_mainBus.write(m_addrAbs, m_registers.x);
    return 0;
}

uint8_t MOS6502::STY(){

    m_mainBus.write(m_addrAbs, m_registers.y);
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
    m_registers.status.c = (m_mainBus.read(m_addrAbs) & 0x80) >> 7;

    return 0;
}

uint8_t MOS6502::ANE(){

    m_registers.acc |= 0xFF;
    m_registers.acc &= m_registers.x;
    m_registers.acc &= m_mainBus.read(m_addrAbs);

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

    uint8_t value = m_mainBus.read(m_addrAbs) & m_registers.sp;
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
    m_registers.acc &= m_mainBus.read(m_addrAbs);
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

    m_mainBus.write(m_addrAbs, m_registers.acc & m_registers.x);
    return 0;
}

uint8_t MOS6502::SBX(){

    m_registers.x = (m_registers.acc & m_registers.x) - m_mainBus.read(m_addrAbs);
    m_registers.status.n = (m_registers.x & 0x80) == 0x80;
    m_registers.status.z = m_registers.x == 0x0;
    m_registers.status.c = (m_registers.acc >= m_registers.x);
    return 0;
}

uint8_t MOS6502::SHA(){

    m_mainBus.write(m_addrAbs, m_registers.acc & m_registers.x & ((m_addrAbs & 0xFF00 >> 8) + 1));
    return 0;
}

uint8_t MOS6502::SHX(){

    m_mainBus.write(m_addrAbs, m_registers.x & (((m_addrRel & 0xFF00) >> 8) + 1));
    return 0;
}

uint8_t MOS6502::SHY(){

    m_mainBus.write(m_addrAbs, m_registers.y & (((m_addrRel & 0xFF00) >> 8) + 1));
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
    m_mainBus.write(m_addrAbs, m_registers.acc & m_registers.x & (((m_addrAbs & 0xFF00) >> 8) + 1));

    return 0;
}

uint8_t MOS6502::JAM(){

    m_registers.pc--; //Decreases the PC to the itself, effectively stopping the execution process.
    return 0;
}

void MOS6502::init() {
    hardReset();
}


