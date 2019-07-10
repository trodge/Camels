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

class Printer {
    std::vector<int> sizes;
    std::vector<int>::difference_type sizeIndex;
    unsigned int nationId;
    SDL_Color foreground, background, highlight;
    int highlightLine;
    std::vector<sdl::FontPtr> fonts;

  public:
    void setSize(int s);
    void setColors(const SDL_Color &fg, const SDL_Color &bg) {
        foreground = fg;
        background = bg;
    }
    void setHighlight(const SDL_Color &hl) { highlight = hl; };
    void setHighlightLine(int h) { highlightLine = h; }
    void setNationId(unsigned int n) { nationId = n; }
    int getFontHeight() { return TTF_FontHeight(fonts[static_cast<size_t>(sizeIndex * kFontCount)].get()); }
    int getFontWidth(const std::string &tx);
    sdl::SurfacePtr print(const std::vector<std::string> &tx, SDL_Rect &rt, int b, int r, SDL_Surface *img);
    sdl::SurfacePtr print(const std::vector<std::string> &tx, SDL_Rect &rt, int b, int r) {
        return print(tx, rt, b, r, nullptr);
    }
};

#endif
