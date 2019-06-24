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

#include <SDL2/SDL_ttf.h>

#include "constants.h"
#include "draw.h"

class Printer {
    static std::vector<int> sizes;
    static size_t sizeIndex;
    static int nationId;
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
    static void setNationId(int n) { nationId = n; }
    static int getFontHeight() { return TTF_FontHeight(fonts[sizeIndex * kFontCount]); }
    static int getFontWidth(const std::string &tx);
    static SDL_Surface *print(const std::vector<std::string> &tx, int w, int h, int b, int r);
};

#endif
