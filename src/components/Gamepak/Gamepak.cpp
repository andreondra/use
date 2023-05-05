/**
 * @file Gamepak.cpp
 * @author Ondrej Golasowski (golasowski.o@gmail.com)
 * @brief NES Gamepak emulation implementation.
 * @copyright Copyright (c) 2022 Ondrej Golasowski
 *
 */

#include <memory>
#include <string>
#include "imgui.h"
#include "ImGuiFileDialog.h"

#include "components/Gamepak/Gamepak.h"
#include "Connector.h"
#include "components/Gamepak/Mapper.h"
#include "components/Gamepak/Mapper000.h"
#include "components/Gamepak/Mapper001.h"

Gamepak::Gamepak() {

    m_deviceName = "Gamepak";

    Connector cpuConnector(DataInterface{
        .read = [&](uint32_t address, uint32_t & buffer)-> bool {

            if(m_mapper) {
                uint8_t mapperResult;
                bool mapperState = m_mapper->cpuRead(static_cast<uint16_t>(address), mapperResult);
                buffer = mapperResult;
                return mapperState;
            } else
                return false;
        },

        .write = [&](uint32_t address, uint32_t data) {
            if(m_mapper)
                m_mapper->cpuWrite(static_cast<uint16_t>(address), static_cast<uint8_t>(data));
        }
    });

    Connector ppuConnector(DataInterface{
            .read = [&](uint32_t address, uint32_t & buffer)-> bool {

                if(m_mapper) {

                    uint8_t mapperResult;
                    bool mapperState = m_mapper->ppuRead(static_cast<uint16_t>(address), mapperResult);
                    buffer = mapperResult;
                    return mapperState;

                } else {
                    return false;
                }
            },

            .write = [&](uint32_t address, uint32_t data) {
                if(m_mapper) {
                    m_mapper->ppuWrite(static_cast<uint16_t>(address), static_cast<uint8_t>(data));
                }
            }
    });

    m_connectors["cpuBus"] = std::make_shared<Connector>(cpuConnector);
    m_connectors["ppuBus"] = std::make_shared<Connector>(ppuConnector);
}

Gamepak::~Gamepak() = default;

void Gamepak::init(){

    if(m_mapper)
        m_mapper->init();
}

void Gamepak::load(std::ifstream & ifs){

    // Clear data.
    m_mapper.reset();
    m_trainer.clear();
    m_PRGROM.clear();
    m_CHRROM.clear();

    // Init metadata to values common for all iNES file types.
    m_params.init();

    // Buffer for temp storage.
    std::vector<char> buffer;

    // Check file signature.
    // ============================================================
    buffer.resize(4);
    ifs.read(buffer.data(), 4);
    if(!ifs || ifs.gcount() != 4) throw std::runtime_error("File I/O error.");
    // Expected signature is "NES" in ASCII followed by MS-DOS EOF.
    if(buffer != std::vector<char>({0x4E, 0x45, 0x53, 0x1A}))
        throw std::invalid_argument("File has malformed header.");

    // Load PRG (program) and CHR (character) ROM size.
    // ============================================================
    buffer.resize(2);
    ifs.read(buffer.data(), 2);
    if(!ifs || ifs.gcount() != 2) throw std::runtime_error("File I/O error.");
    m_params.PRGROMsize = PRGROM_UNIT_SIZE * static_cast<uint8_t>(buffer[0]);
    m_params.CHRROMsize = CHRROM_UNIT_SIZE * static_cast<uint8_t>(buffer[1]);

    // Read header.
    // ============================================================
    buffer.resize(10);
    ifs.read(buffer.data(), 10);
    if(!ifs || ifs.gcount() != 10) throw std::runtime_error("File I/O error.");

    uint8_t flags6 = buffer[0];
    uint8_t flags7 = buffer[1];
    uint8_t flags8 = buffer[2];
    uint8_t flags9 = buffer[3];
    uint8_t flags10 = buffer[4];
    uint8_t flags11 = buffer[5];
    uint8_t flags12 = buffer[6];
    uint8_t flags13 = buffer[7];
    uint8_t flags14 = buffer[8];
    uint8_t flags15 = buffer[9];

    // Choosing mirroring type.
    bool ignoreMirroring = flags6 & 0x8;
    if(ignoreMirroring) {
        m_params.mirroringType = Mapper::mirroringType_t::FOURSCREEN;
    } else {
        bool mirroring = flags6 & 0x01;
        m_params.mirroringType = (mirroring) ? Mapper::mirroringType_t::VERTICAL : Mapper::mirroringType_t::HORIZONTAL;
    }

    m_params.hasPersistentMemory = flags6 & 0x2;
    m_params.hasTrainer = flags6 & 0x4;
    m_params.mapperNumber = (flags6 & 0xF0) >> 4;

    // Determining file format.
    uint8_t formatFlag = (flags7 & 0x0C);
    if(formatFlag == 0x08){
        m_params.fileFormat = fileFormat_t::NES20;
    } else if(formatFlag == 0x04) {
        m_params.fileFormat = fileFormat_t::ARCHAICINES;
    } else if (
        formatFlag == 0x00 &&
        0 == flags12     &&
        0 == flags13     &&
        0 == flags14     &&
        0 == flags15
    ){
        m_params.fileFormat = fileFormat_t::INES;
    // Probably archaic iNES or iNES 0.7.
    } else {
        m_params.fileFormat = fileFormat_t::ARCHAICINES;
    }

    // -------------------------------------------------------------
    // End of common meta for all iNES types.

    if(m_params.fileFormat == fileFormat_t::INES || m_params.fileFormat == fileFormat_t::NES20) {
        uint8_t consoleType = flags7 & 0x3;

        if(consoleType == 0x0)      m_params.consoleType == consoleType_t::STANDARD;
        else if(consoleType == 0x1) m_params.consoleType == consoleType_t::VSUNISYSTEM;
        else if(consoleType == 0x2) m_params.consoleType == consoleType_t::PLAYCHOICE;
        else                        m_params.consoleType == consoleType_t::EXTENDED;

        m_params.mapperNumber |= flags7 & 0xF0;
    }

    // Parse rest of the header in the corresponding format.
    if(m_params.fileFormat == fileFormat_t::INES) {

        // PRG RAM unit count. Value 0 infers 8 KiB for compatibility.
        // See https://www.nesdev.org/wiki/INES
        m_params.PRGRAMsize = ((flags8 == 0) ? 1 : flags8) * PRGRAM_UNIT_SIZE;

        if(flags9 & 0x1) {
            m_params.tvSystem = tvSystem_t::PAL;
            throw std::invalid_argument("PAL systems not yet supported.");
        } else {
            m_params.tvSystem = tvSystem_t::NTSC;
        }

        // Flags 10 ignored, because they are not included in the official specification.

    } else if(m_params.fileFormat == fileFormat_t::NES20) {

        m_params.mapperNumber |= (flags8 & 0x0F) << 8;
        m_params.submapperNumber = (flags8 & 0xF0) >> 4;

        m_params.PRGROMsize |= (flags9 & 0x0F) << 8;
        m_params.CHRROMsize |= (flags9 & 0xF0) << 4;

        m_params.PRGRAMsize = ((flags10 & 0x0F) == 0) ? 0 : (64 << (flags10 & 0x0F));
        m_params.PRGNVRAMsize = ((flags10 & 0xF0) == 0) ? 0 : (64 << ((flags10 & 0xF0) >> 4));

        m_params.CHRRAMsize = ((flags11 & 0x0F) == 0) ? 0 : (64 << (flags11 & 0x0F));
        m_params.CHRNVRAMsize = ((flags11 & 0xF0) == 0) ? 0 : (64 << ((flags11 & 0xF0) >> 4));

        switch(flags12 & 0x3) {
            case 0: m_params.tvSystem == tvSystem_t::NTSC;
            default: throw std::invalid_argument("NTSC systems supported only.");
//            case 1: m_params.tvSystem == tvSystem_t::PAL;
//            case 2: m_params.tvSystem == tvSystem_t::MULTI;
//            case 3: m_params.tvSystem == tvSystem_t::DENDY;
        }

        // Flags 13, 14, 15 not yet supported.

    } else {
        // Nothing to do.
    }

    // Read trainer data (if present).
    // ============================================================
    if(m_params.hasTrainer){

        m_trainer.resize(512);
        ifs.read((char*)m_trainer.data(), 512);
        if(!ifs || ifs.gcount() != 512)
            throw std::runtime_error("File I/O error.");
    }

    // Load PRG ROM contents.
    // ============================================================
    if(m_params.PRGROMsize > 0) {
        m_PRGROM.resize(m_params.PRGROMsize);
        ifs.read((char*)m_PRGROM.data(), m_params.PRGROMsize);
        if(!ifs || ifs.gcount() != m_params.PRGROMsize)
            throw std::runtime_error("File I/O error.");
    }

    // Load CHR ROM contents.
    // ============================================================
    if(m_params.CHRROMsize > 0) {
        m_CHRROM.resize(m_params.CHRROMsize);
        ifs.read((char*)m_CHRROM.data(), m_params.CHRROMsize);
        if(!ifs || ifs.gcount() != m_params.CHRROMsize)
            throw std::runtime_error("File I/O error.");
    }

    // Load Playchoice data.
    // ============================================================
    // Not yet supported.

    //  File parsing complete, assigning mapper.
    // ============================================================
    switch(m_params.mapperNumber){

        case 0x0000: m_mapper = std::make_unique<Mapper000>(m_PRGROM, m_CHRROM, m_params.mirroringType); break;
        case 0x0001: m_mapper = std::make_unique<Mapper001>(m_PRGROM, m_CHRROM, m_params.PRGRAMsize); break;

        using namespace std::literals;
        default: throw std::runtime_error("Mapper "s + std::to_string(m_params.mapperNumber) + " is not supported.");
    }

    // Get mapper's VRAM mode.
    // a) use CIRAM with fixed mirroring (soldered pad on original HW)
    // b) use CIRAM with switchable mirroring
    // c) use own VRAM
}

std::vector<EmulatorWindow> Gamepak::getGUIs() {

    std::vector<EmulatorWindow> windows;

    std::function<void(void)> gamepakGUI = [this](){

        // Local data
        // ===================================================================
        static std::string modalText;

        // Window contents
        // ===================================================================
        ImGui::SeparatorText("Load from file");
        if(ImGui::Button("Select file")) {
            ImGuiFileDialog::Instance()->OpenDialog("ChooseGamepakFileLoad", "Choose File", ".nes", ".");
        }

        // Dialogs
        // ===================================================================
        if(ImGuiFileDialog::Instance()->Display("ChooseGamepakFileLoad")) {

            if(ImGuiFileDialog::Instance()->IsOk()) {

                std::string filePath = ImGuiFileDialog::Instance()->GetFilePathName();

                std::ifstream file(filePath, std::ios_base::binary);
                if(!file) {
                    modalText = "Specified file couldn't be opened!";
                    ImGui::OpenPopup(std::string(m_deviceName + ": Error").data());
                } else {

                    try {
                        load( file);
                    } catch (std::invalid_argument & e) {
                        modalText = "Specified file is malformed: " + std::string(e.what() ? e.what() : "");
                        ImGui::OpenPopup(std::string(m_deviceName + ": Error").data());
                    } catch (std::runtime_error & e) {
                        modalText = "Specified file couldn't be opened: " + std::string(e.what() ? e.what() : "");
                        ImGui::OpenPopup(std::string(m_deviceName + ": Error").data());
                    } catch (...) {
                        modalText = "Other error loading the file.";
                        ImGui::OpenPopup(std::string(m_deviceName + ": Error").data());
                    }

                    m_initRequested = true;
                }
            }

            ImGuiFileDialog::Instance()->Close();
        }

        if(ImGui::BeginPopupModal(std::string(m_deviceName + ": Error").data(), nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
            ImGui::Text("%s", modalText.c_str());
            ImGui::Separator();
            if (ImGui::Button("OK")) { ImGui::CloseCurrentPopup(); }
            ImGui::SetItemDefaultFocus();
            ImGui::EndPopup();
        }

        // Draw additional mapper's GUI if available.
        if(m_mapper) {
            m_mapper->drawGUI();
        }
    };

    return {
            EmulatorWindow{
                    .category = m_deviceName,
                    .title = "Parameters",
                    .id    = getDeviceID(),
                    .dock  = DockSpace::LEFT,
                    .guiFunction = gamepakGUI
            }
    };
}