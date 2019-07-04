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

#include "loadbar.hpp"

LoadBar::LoadBar(SDL_Rect rt, const std::vector<std::string> &t, SDL_Color fg, SDL_Color bg, int b, int r, int fS)
    : TextBox({rt.x + rt.w / 2, rt.y + rt.h / 2, 0, 0}, t, fg, bg, 0, r, fS),
      completeRect({rt.x + b, rt.y + b, 0, rt.h - 2 * b}), insideRect({rt.x + b, rt.y + b, rt.w - 2 * b, rt.h - 2 * b}),
      outsideRect(rt), outsideBorder(b) {}

LoadBar::~LoadBar() {}

void LoadBar::progress(double c) {
    if (complete + c < 1)
        complete += c;
    else
        complete = 1;
    completeRect.w = static_cast<int>((outsideRect.w - 2 * outsideBorder) * complete);
}

void LoadBar::draw(SDL_Renderer *s) {
    // draw border
    drawRoundedRectangle(s, radius, &outsideRect, foreground);
    // draw incomplete rectangle
    drawRoundedRectangle(s, radius, &insideRect, background);
    // draw complete rectangle
    drawRoundedRectangle(s, radius, &completeRect, foreground);
    TextBox::draw(s);
}
