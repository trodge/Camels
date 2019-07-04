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

#ifndef CONSTANTS_H
#define CONSTANTS_H

#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>

const double kShowPlayerPadding = 0.2; // Portion of screen to pad around player when map zooms to player
const size_t kMaxTowns = 500;          // Maximum number of towns allowed to be loaded
const int kDaysPerYear = 365;
const std::vector<TTF_Font *>::difference_type kFontCount = 5; // Number of fonts used to display text
const size_t kTradePortionIndex = 1;                           // Index of text box representing trade portion
const size_t kOfferButtonIndex = 4;                            // The index of first offer button when trading
const size_t kFightChoiceIndex = 3;                            // Index of box representing choice when fighting

/* SDL interprets each pixel as a 32-bit number, so our masks must depend
   on the endianness (byte order) of the machine */
#if SDL_BYTEORDER == SDL_BIG_ENDIAN
const Uint32 rmask = 0xff000000;
const Uint32 gmask = 0x00ff0000;
const Uint32 bmask = 0x0000ff00;
const Uint32 amask = 0x000000ff;
#else
const Uint32 rmask = 0x000000ff;
const Uint32 gmask = 0x0000ff00;
const Uint32 bmask = 0x00ff0000;
const Uint32 amask = 0xff000000;
#endif

#endif