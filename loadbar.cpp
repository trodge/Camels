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
 
#include "loadbar.h"
LoadBar::LoadBar(SDL_Rect rt, const std::vector<std::string> &t, SDL_Color fg, SDL_Color bg, int b, int r, int fS)
    : TextBox({rt.x + rt.w / 2, rt.y + rt.h / 2}, t, fg, bg, 0, r, fS), completeRect({rt.x + b, rt.y + b, 0, rt.h - 2 * b}),
      insideRect({rt.x + b, rt.y + b, rt.w - 2 * b, rt.h - 2 * b}), outsideRect(rt), outsideBorder(b) {}

LoadBar::~LoadBar() {}

void LoadBar::progress(double c) {
    if (complete + c < 1)
        complete += c;
    else
        complete = 1;
    completeRect.w = (outsideRect.w - 2 * outsideBorder) * complete;
}

void LoadBar::draw(SDL_Surface *s) {
    Uint32 c = SDL_MapRGB(s->format, foreground.r, foreground.g, foreground.b);
    Uint32 b = SDL_MapRGB(s->format, background.r, background.g, background.b);
    // draw border
    SDL_FillRect(s, &outsideRect, c);
    // draw incomplete rectangle
    SDL_FillRect(s, &insideRect, b);
    // draw complete rectangle
    SDL_FillRect(s, &completeRect, c);
    TextBox::draw(s);
}
