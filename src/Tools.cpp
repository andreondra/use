#include "imgui.h"
#include "Types.h"
#include <cmath>

namespace USETools {

    void renderScalableBitmap(const std::vector<std::vector<RGBPixel>> & pixelData, float scale) {

        if(pixelData.empty())
            throw std::invalid_argument("Pixel data can't be empty!");

        // Height = count of rows.
        size_t height = pixelData.size();

        // Width = size of the largest row; has to be calculated.
        size_t width = 0;

        size_t currentX = 0;
        size_t currentY = 0;

        ImDrawList * dl = ImGui::GetWindowDrawList();
        const ImVec2 defaultScreenPos = ImGui::GetCursorScreenPos();

        // Render by rows, which contain pixels.
        for(auto & row : pixelData) {

            currentX = 0;
            for(auto & pixel : row) {

                ImColor color = ImColor(
                        pixel.red,
                        pixel.green,
                        pixel.blue,
                        255
                );

                float screenX = defaultScreenPos.x + scale * static_cast<float>(currentX);
                float screenY = defaultScreenPos.y + scale * static_cast<float>(currentY);
                dl->AddRectFilled({screenX, screenY}, {screenX + scale, screenY + scale}, color);

                currentX++;
            }

            // Check if the row is the largest.
            width = std::max(pixelData[0].size(), width);

            currentY++;
        }

        float dummyWidth = scale * static_cast<float>(width);
        float dummyHeight = scale * static_cast<float>(height);

        // Dummy widget is needed for scrollbars to work and to allow to place more elements below correctly.
        ImGui::Dummy({dummyWidth, dummyHeight});
    }

    double map(double val, double iStart, double iEnd, double oStart, double oEnd){

        return oStart + std::round((oEnd - oStart) / (iEnd - iStart)) * (val - iStart);
    }
}