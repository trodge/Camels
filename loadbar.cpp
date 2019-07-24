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

LoadBar::LoadBar(const BoxInfo &bI, Printer &pr)
    : TextBox{bI, pr}, completeRect({bI.outsideRect.x + bI.border, bI.outsideRect.y + bI.border, 0, bI.outsideRect.h - 2 * bI.border}),
      insideRect({bI.outsideRect.x + bI.border, bI.outsideRect.y + bI.border, bI.outsideRect.w - 2 * bI.border, bI.outsideRect.h - 2 * bI.border}), outsideRect(bI.outsideRect), outsideBorder(bI.border) {}

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
