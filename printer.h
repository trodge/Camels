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

#include "constants.h"
#include "draw.h"

class Printer {
    static std::vector<int> sizes;
    static std::vector<int>::difference_type sizeIndex;
    static unsigned int nationId;
    static SDL_Color foreground, background, highlight;
    static int highlightLine;
    static std::vector<TTF_Font *> fonts;

  public:
    static void closeFonts();
    static void setSize(int s);
    static void setColors(const SDL_Color &fg, const SDL_Color &bg) {
        foreground = fg;
        background = bg;
    }
    static void setHighlight(const SDL_Color &hl) { highlight = hl; };
    static void setHighlightLine(int h) { highlightLine = h; }
    static void setNationId(unsigned int n) { nationId = n; }
    static int getFontHeight() { return TTF_FontHeight(fonts[static_cast<size_t>(sizeIndex * kFontCount)]); }
    static int getFontWidth(const std::string &tx);
    static SDL_Surface *print(const std::vector<std::string> &tx, SDL_Rect &rt, int b, int r, SDL_Surface *img);
    static SDL_Surface *print(const std::vector<std::string> &tx, SDL_Rect &rt, int b, int r) {
        return print(tx, rt, b, r, nullptr);
    }
};

#endif
