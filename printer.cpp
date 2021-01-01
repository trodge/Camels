/*
 * This file is part of Camels.
 *
 * Camels is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Camels is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Camels.  If not, see <https://www.gnu.org/licenses/>.
 *
 * © Tom Rodgers notaraptor@gmail.com 2017-2019
 */

#include "printer.hpp"

void Printer::setSize(BoxSize sz) {
    size = sz;
    unsigned int fS = sz.fontSize;
    fontSizeIt = std::lower_bound(begin(fontSizes), end(fontSizes), fS); // iterator to correct font size
    if (fontSizeIt == end(fontSizes) || fontSizeIt->size != fS) {
        fontSizeIt = fontSizes.insert(
            fontSizeIt, {{sdl::openFont("DejaVuSerif.ttf", fS), sdl::openFont("DejaVuSans.ttf", fS),
                          sdl::openFont("NotoSerifDevanagari-Regular.ttf", fS),
                          sdl::openFont("wqy-microhei-lite.ttc", fS),
                          sdl::openFont("NotoSerifBengali-Regular.ttf", fS)},
                         fS});
    }
}

void Printer::setColors(const ColorScheme &clrs) {
    colors = clrs;
    if (clrs.invert) std::swap(colors.foreground, colors.background);
}

int Printer::getFontWidth(const std::string &tx) {
    int w;
    TTF_SizeUTF8(fontSizeIt->fonts.front().get(), tx.c_str(), &w, nullptr);
    return w;
}

sdl::Surface Printer::print(const std::vector<std::string> &tx, SDL_Rect &rt, const std::vector<Image> &imgs) {
    // Create a new SDL_Surface with the given text printed on it. Nation ID used to determine fonts.
    size_t numLines = tx.size();
    std::vector<int> tWs, tHs;      // Widths and heights for each line of text.
    std::vector<SDL_Surface *> tSs; // Surfaces containing rendered lines of text.
    tWs.reserve(numLines);
    tHs.reserve(numLines);
    tSs.reserve(numLines);
    int mW = 0;                                                                // Minimum width to fit text.
    int mH = 0;                                                                // Minimum height to fit text.
    std::array<sdl::Font, kFontCount>::iterator fI = begin(fontSizeIt->fonts); // Font to use.
    for (auto &t : tx) {
        // Render lines of text.
        if (t.empty())
            tSs.push_back(TTF_RenderUTF8_Solid(fI->get(), " ", colors.foreground));
        else
            tSs.push_back(TTF_RenderUTF8_Solid(fI->get(), t.c_str(), colors.foreground));
        tWs.push_back(tSs.back()->w);
        if (tWs.back() > mW) mW = tWs.back();
        tHs.push_back(TTF_FontHeight(fI->get()));
        mH += tHs.back();
        switch (nationId) {
        case 0:
            break;
        case 5:
        case 9:
            fI += 2;
            break;
        case 7:
        case 8:
            fI += 3;
            break;
        case 10:
            fI += 4;
            break;
        default:
            fI += 1;
        }
    }
    if (!rt.w) rt.w = mW + 2 * size.border;
    if (!rt.h) rt.h = mH + 2 * size.border;

    sdl::Surface p(SDL_CreateRGBSurface(surfaceFlags, rt.w, rt.h, bitDepth, rmask, gmask, bmask, amask));
    sdl::Renderer swRdr(SDL_CreateSoftwareRenderer(p.get()));
    SDL_Rect dR = {0, 0, rt.w, rt.h};
    if (size.border) {
        // Draw border.
        drawRoundedRectangle(swRdr.get(), size.radius, &dR, colors.foreground);
        dR = {size.border, size.border, rt.w - 2 * size.border, rt.h - 2 * size.border};
    }
    // Draw background.
    drawRoundedRectangle(swRdr.get(), size.radius, &dR, colors.background);
    Alignment justify = Alignment::center;
    for (auto &img : imgs) {
        // Blit image onto its rectangle on printing surface.
        dR = img.rect;
        if (dR.x + dR.w / 2 < rt.w / 2)
            // Image is on the left side, justify text right.
            justify = Alignment::right;
        else
            // Image is on right side, justify text left.
            justify = Alignment::left;
        SDL_BlitSurface(img.surface, nullptr, p.get(), &dR);
    }
    // Center text vertically.
    dR.y = rt.h / 2 - mH / 2;
    for (size_t i = 0; i < numLines; ++i) {
        // Blit rendered text lines onto one surface.
        dR.w = tWs[i];
        dR.h = tHs[i];
        switch (justify) {
        case Alignment::left:
            dR.x = 2 * size.border;
            break;
        case Alignment::right:
            dR.x = rt.w - dR.w - 2 * size.border;
            break;
        case Alignment::center:
            dR.x = rt.w / 2 - dR.w / 2;
            break;
        }
        if (i == static_cast<size_t>(highlightLine)) {
            SDL_Rect hlR = {size.border, dR.y, rt.w - 2 * size.border, dR.h};
            SDL_SetRenderDrawColor(swRdr.get(), colors.highlight.r, colors.highlight.g, colors.highlight.b,
                                   colors.highlight.a);
            SDL_RenderFillRect(swRdr.get(), &hlR);
        }
        SDL_BlitSurface(tSs[i], nullptr, p.get(), &dR);
        SDL_FreeSurface(tSs[i]);
        dR.y += dR.h;
    }
    return p;
}
