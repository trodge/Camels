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

#ifndef PRINTER_H
#define PRINTER_H

#include <algorithm>
#include <iostream>
#include <string>
#include <vector>

#include "constants.hpp"
#include "draw.hpp"
#include "settings.hpp"

class Printer {
    unsigned int nationId;
    SDL_Color foreground, background, highlight;
    int highlightLine = -1;
    struct FontSize {
        bool operator<(unsigned int sz) { return size < sz; }
        std::array<sdl::Font, kFontCount> fonts;
        unsigned int size;
    };
    std::vector<FontSize> fontSizes;
    std::vector<FontSize>::iterator fontSizeIt;
    enum Alignment { left, right, center };

public:
    void setSize(unsigned int s);
    void setColors(const SDL_Color &fg, const SDL_Color &bg) {
        foreground = fg;
        background = bg;
    }
    void setHighlight(const SDL_Color &hl) { highlight = hl; };
    void setHighlightLine(int h) { highlightLine = h; }
    void setNationId(unsigned int n) { nationId = n; }
    int getFontHeight() { return TTF_FontHeight(fontSizeIt->fonts.front().get()); }
    int getFontWidth(const std::string &tx);
    sdl::Surface print(const std::vector<std::string> &tx, SDL_Rect &rt, int b, int r, const std::vector<Image> &imgs);
    sdl::Surface print(const std::vector<std::string> &tx, SDL_Rect &rt, int b, int r) {
        return print(tx, rt, b, r, std::vector<Image>());
    }
};

#endif
