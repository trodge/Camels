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

#ifndef LOADBAR_H
#define LOADBAR_H

#include "draw.hpp"
#include "textbox.hpp"

class LoadBar : public TextBox {
    double complete = 0;
    SDL_Rect completeRect, insideRect, outsideRect;
    int outsideBorder;

  public:
    LoadBar(const SDL_Rect &rt, const std::vector<std::string> &t, const SDL_Color &fg, const SDL_Color &bg, int b, int r,
            int fS);
    void progress(double c);
    void draw(SDL_Renderer *s);
};

#endif // LOADBAR_H
