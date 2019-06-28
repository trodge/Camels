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

#include "printer.h"

std::vector<int> Printer::sizes;
std::vector<int>::difference_type Printer::sizeIndex;
unsigned int Printer::nationId;
SDL_Color Printer::foreground, Printer::background, Printer::highlight;
int Printer::highlightLine = -1;
std::vector<TTF_Font *> Printer::fonts;

void Printer::closeFonts() {
    for (auto &f : fonts) {
        TTF_CloseFont(f);
    }
}

void Printer::setSize(int s) {
    sizeIndex = std::lower_bound(sizes.begin(), sizes.end(), s) - sizes.begin();
    if (static_cast<size_t>(sizeIndex) == sizes.size() or sizes[static_cast<size_t>(sizeIndex)] != s) {
        sizes.insert(sizes.begin() + sizeIndex, s);
        fonts.insert(fonts.begin() + sizeIndex * kFontCount, TTF_OpenFont("DejaVuSerif.ttf", s));
        fonts.insert(fonts.begin() + sizeIndex * kFontCount + 1, TTF_OpenFont("DejaVuSans.ttf", s));
        fonts.insert(fonts.begin() + sizeIndex * kFontCount + 2, TTF_OpenFont("NotoSerifDevanagari-Regular.ttf", s));
        fonts.insert(fonts.begin() + sizeIndex * kFontCount + 3, TTF_OpenFont("wqy-microhei-lite.ttc", s));
        fonts.insert(fonts.begin() + sizeIndex * kFontCount + 4, TTF_OpenFont("NotoSerifBengali-Regular.ttf", s));
    }
}

int Printer::getFontWidth(const std::string &tx) {
    int w;
    TTF_SizeUTF8(fonts[static_cast<size_t>(sizeIndex * kFontCount)], tx.c_str(), &w, nullptr);
    return w;
}

SDL_Surface *Printer::print(const std::vector<std::string> &tx, SDL_Rect &rt, int b, int r) {
    // Create a new SDL_Surface with the given text printed on it. Nation ID used to determine fonts.
    size_t numLines = tx.size();
    std::vector<int> tWs, tHs;      // Widths and heights for each line of text.
    std::vector<SDL_Surface *> tSs; // Surfaces containing rendered lines of text.
    tWs.reserve(numLines);
    tHs.reserve(numLines);
    tSs.reserve(numLines);
    int mW = 0;                                                                    // Minimum width to fit text.
    int mH = 0;                                                                    // Minimum height to fit text.
    std::vector<TTF_Font *>::iterator fI = fonts.begin() + sizeIndex * kFontCount; // Font to use.
    for (auto &t : tx) {
        // Render lines of text.
        if (t.empty())
            tSs.push_back(TTF_RenderUTF8_Solid(*fI, " ", foreground));
        else
            tSs.push_back(TTF_RenderUTF8_Solid(*fI, t.c_str(), foreground));
        tWs.push_back(tSs.back()->w);
        if (tWs.back() > mW)
            mW = tWs.back();
        tHs.push_back(TTF_FontHeight(*fI));
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
    if (not rt.w)
        rt.w = mW + 2 * b;
    if (not rt.h)
        rt.h = mH + 2 * b;

    Uint32 rmask, gmask, bmask, amask;
    /* SDL interprets each pixel as a 32-bit number, so our masks must depend
       on the endianness (byte order) of the machine */
#if SDL_BYTEORDER == SDL_BIG_ENDIAN
    rmask = 0xff000000;
    gmask = 0x00ff0000;
    bmask = 0x0000ff00;
    amask = 0x000000ff;
#else
    rmask = 0x000000ff;
    gmask = 0x0000ff00;
    bmask = 0x00ff0000;
    amask = 0xff000000;
#endif

    SDL_Surface *p = SDL_CreateRGBSurface(0, rt.w, rt.h, 32, rmask, gmask, bmask, amask);
    SDL_Renderer *s = SDL_CreateSoftwareRenderer(p);
    // Draw border.
    SDL_Rect dR = rt;
    if (b) {
        drawRoundedRectangle(s, r, &dR, foreground);
    }
    dR = {b, b, rt.w - 2 * b, rt.h - 2 * b};
    drawRoundedRectangle(s, r, &dR, background);
    // Center text vertically.
    dR.y = rt.h / 2 - mH / 2;
    for (size_t i = 0; i < numLines; ++i) {
        // Blit rendered text lines onto one surface.
        dR.w = tWs[i];
        dR.h = tHs[i];
        // Center text horizontally
        dR.x = rt.w / 2 - dR.w / 2;
        if (i == size_t(highlightLine)) {
            SDL_Rect hlR = {b, dR.y, rt.w - 2 * b, dR.h};
            SDL_SetRenderDrawColor(s, highlight.r, highlight.g, highlight.b, highlight.a);
            SDL_RenderFillRect(s, &hlR);
        }
        SDL_BlitSurface(tSs[i], NULL, p, &rt);
        SDL_FreeSurface(tSs[i]);
        rt.y += rt.h;
    }
    SDL_DestroyRenderer(s);
    return p;
}
