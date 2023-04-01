/**
 * @file Tools.h
 * @author Ondrej Golasowski (golasowski.o@gmail.com)
 * @brief Helper functions.
 * @copyright Copyright (c) 2023 Ondrej Golasowski
 *
 */

#ifndef USE_TOOLS_H
#define USE_TOOLS_H

#include "imgui.h"
#include "Types.h"

namespace USETools {

    /**
     * Render a bitmap.
     *
     * @param pixelData Bitmap data in a form of: vector of rows, which are vectors of pixels (rows->row->pixel) => pixelData[y][x].
     * @param scale Scale of the bitmap.
     *
     * @note Rows can be non-homogenous (having every row of a different size is allowed).
     * @throw std::invalid_argument When pixel data is empty.
     * */
    void renderScalableBitmap(const std::vector<std::vector<RGBPixel>> & pixelData, float scale = 1.0f);

    double map(double val, double iStart, double iEnd, double oStart, double oEnd);
}

#endif //USE_TOOLS_H
