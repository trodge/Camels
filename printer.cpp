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

void Printer::setSize(unsigned int sz) {
    fontSizeIt = std::lower_bound(fontSizes.begin(), fontSizes.end(), sz); // iterator to correct font size
    if (fontSizeIt == fontSizes.end() || fontSizeIt->size != sz) {
        fontSizeIt = fontSizes.insert(
            fontSizeIt, {{sdl::openFont("DejaVuSerif.ttf", sz), sdl::openFont("DejaVuSans.ttf", sz),
                          sdl::openFont("NotoSerifDevanagari-Regular.ttf", sz),
                          sdl::openFont("wqy-microhei-lite.ttc", sz), sdl::openFont("NotoSerifBengali-Regular.ttf", sz)},
                         sz});
    }
}

int Printer::getFontWidth(const std::string &tx) {
    int w;
    TTF_SizeUTF8(fontSizeIt->fonts.front().get(), tx.c_str(), &w, nullptr);
    return w;
}

sdl::SurfacePtr Printer::print(const std::vector<std::string> &tx, SDL_Rect &rt, int b, int r, SDL_Surface *img) {
    // Create a new SDL_Surface with the given text printed on it. Nation ID used to determine fonts.
    size_t numLines = tx.size();
    std::vector<int> tWs, tHs;      // Widths and heights for each line of text.
    std::vector<SDL_Surface *> tSs; // Surfaces containing rendered lines of text.
    tWs.reserve(numLines);
    tHs.reserve(numLines);
    tSs.reserve(numLines);
    int mW = 0;                                                                    // Minimum width to fit text.
    int mH = 0;                                                                    // Minimum height to fit text.
    std::array<sdl::FontPtr, kFontCount>::iterator fI = fontSizeIt->fonts.begin(); // Font to use.
    for (auto &t : tx) {
        // Render lines of text.
        if (t.empty())
            tSs.push_back(TTF_RenderUTF8_Solid(fI->get(), " ", foreground));
        else
            tSs.push_back(TTF_RenderUTF8_Solid(fI->get(), t.c_str(), foreground));
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
    if (!rt.w) rt.w = mW + 2 * b;
    if (!rt.h) rt.h = mH + 2 * b;

    sdl::SurfacePtr p(sdl::makeSurface(rt.w, rt.h));
    sdl::RendererPtr swRdr(sdl::makeSoftwareRenderer(p.get()));
    // Draw border.
    SDL_Rect dR = {0, 0, rt.w, rt.h};
    if (true) { drawRoundedRectangle(swRdr.get(), r, &dR, foreground); }
    dR = {b, b, rt.w - 2 * b, rt.h - 2 * b};
    drawRoundedRectangle(swRdr.get(), r, &dR, background);
    if (img) {
        // Center image vertically and place on left side of text
        dR.w = rt.h - 2 * b;
        dR.h = rt.h - 2 * b;
        dR.x = 2 * b;
        dR.y = rt.h / 2 - dR.h / 2;
        SDL_BlitSurface(img, nullptr, p.get(), &dR);
    }
    // Center text vertically.
    dR.y = rt.h / 2 - mH / 2;
    for (size_t i = 0; i < numLines; ++i) {
        // Blit rendered text lines onto one surface.
        dR.w = tWs[i];
        dR.h = tHs[i];
        if (img)
            // Justify text right
            dR.x = rt.w - dR.w - 2 * b;
        else
            // Center text horizontally
            dR.x = rt.w / 2 - dR.w / 2;
        if (i == size_t(highlightLine)) {
            SDL_Rect hlR = {b, dR.y, rt.w - 2 * b, dR.h};
            SDL_SetRenderDrawColor(swRdr.get(), highlight.r, highlight.g, highlight.b, highlight.a);
            SDL_RenderFillRect(swRdr.get(), &hlR);
        }
        SDL_BlitSurface(tSs[i], nullptr, p.get(), &dR);
        SDL_FreeSurface(tSs[i]);
        dR.y += dR.h;
    }
    return p;
}
