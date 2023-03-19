//
// Created by golas on 2.3.23.
//

#include <fstream>
#include "components/Memory.h"
#include "imgui.h"
#include "imgui_memory_editor.h"
#include "ImGuiFileDialog.h"

Memory::Memory(
    size_t size,
    AddressRange addressRange,
    uint8_t defaultValue)

    : m_data(size),
      m_addressRange(addressRange),
      m_defaultValue(defaultValue) {

    m_deviceName = "Memory";

    Connector dataConnector(
        DataInterface{
             .read = [&](uint32_t address, uint32_t & buffer) {

                 if(m_addressRange.has(address)) {
                     buffer = m_data[(address - m_addressRange.from) % m_data.size()];
                     return true;
                 } else {
                     return false;
                 }

             },

             .write = [&](uint32_t address, uint32_t data){
                 if(m_addressRange.has(address))
                     m_data[(address - m_addressRange.from) % m_data.size()] = data;
             }
     });

    m_connectors["data"] = std::make_shared<Connector>(dataConnector);

    memoryInit();
}

void Memory::memoryInit() {
    std::fill(m_data.begin(), m_data.end(), m_defaultValue);
}

void Memory::init() {
    memoryInit();
}

std::vector<EmulatorWindow> Memory::getGUIs() {

    std::function<void(void)> renderContents = [this](){

        // Local data
        // ===================================================================
        static uint32_t loadOffset = 0x00;
        static uint32_t step = 0x1;
        static uint32_t stepFast = 0x10;
        static uint8_t  fillWith = 0xFF;
        static MemoryEditor memoryEditor;

        // Window contents
        // ===================================================================
        ImGui::Text("Parameters");
        ImGui::Text("Size: %lu (0x%x) B", m_data.size());
        ImGui::Text("At addresses: 0x%x to 0x%x", m_addressRange.from, m_addressRange.to);
        ImGui::Separator();

        ImGui::Text("Load from file");
        if(ImGui::InputScalar("Offset", ImGuiDataType_U32, &loadOffset, &step, &stepFast, "%08X")) {
            if(loadOffset >= m_data.size()) {
                loadOffset = 0x00;
            }
        }
        if(ImGui::Button("Select file")) {
            
            ImGuiFileDialog::Instance()->OpenDialog("ChooseMemoryFileLoad", "Choose File", ".bin,.raw", ".");
        }
        ImGui::Separator();

        ImGui::Text("Replace data");
        ImGui::InputScalar("Value", ImGuiDataType_U8, &fillWith, &step, &stepFast, "%08X");
        if(ImGui::Button("Fill")) {
            std::fill(m_data.begin(), m_data.end(), fillWith);
        }
        ImGui::Separator();

        ImGui::Text("Contents");
        memoryEditor.DrawContents(m_data.data(), m_data.size());

        // Dialogs
        // ===================================================================
        if(ImGuiFileDialog::Instance()->Display("ChooseMemoryFileLoad")) {

            if(ImGuiFileDialog::Instance()->IsOk()) {

                std::string filePath = ImGuiFileDialog::Instance()->GetFilePathName();

                std::ifstream file(filePath, std::ios_base::binary);
                if(!file) {
                    ImGui::OpenPopup(std::string(m_deviceName + ": File error").data());
                } else {
                    load(loadOffset, file);
                }
            }

            ImGuiFileDialog::Instance()->Close();
        }

        if(ImGui::BeginPopupModal(std::string(m_deviceName + ": File error").data(), nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
            ImGui::Text("Specified file couldn't be opened!");
            ImGui::Separator();
            if (ImGui::Button("OK")) { ImGui::CloseCurrentPopup(); }
            ImGui::SetItemDefaultFocus();
            ImGui::EndPopup();
        }
    };

    return {
        EmulatorWindow{
            .category = m_deviceName,
            .title = "Debugger",
            .id    = getDeviceID(),
            .dock  = DockSpace::BOTTOM,
            .guiFunction = renderContents
        }
    };
}

void Memory::load(uint32_t startOffset, std::ifstream & src) {

    if(startOffset > m_data.size()) {
        throw std::invalid_argument("Offset is bigger than the memory size.");
    }

    src.read((char *)(m_data.data() + startOffset), m_data.size() - startOffset);
}