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

#ifndef DRAW_H
#define DRAW_H

#include <algorithm>
#include <array>
#include <iostream>

#include <SDL2/SDL.h>

void draw_rounded_rectangle(SDL_Surface *s, int r, SDL_Rect *rect, SDL_Color col);
void draw_circle(SDL_Surface *s, int cx, int cy, int r, SDL_Color col, bool fl);
void draw_circle_symmetry_points(SDL_Surface *s, int cx, int cy, int x, int y, Uint32 color, bool check_bounds);
void draw_line(SDL_Surface *s, int xi, int yi, int xf, int yf, SDL_Color col);
Uint32 get_at(const SDL_Surface *s, int x, int y);
void draw_pixel(SDL_Surface *s, int x, int y, Uint32 color);

#endif
