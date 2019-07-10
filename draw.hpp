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
#include <memory>
#include <vector>

#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <SDL2/SDL_ttf.h>


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


SDL_Texture *textureFromSurfaceSection(SDL_Renderer *rdr, SDL_Surface *sf, const SDL_Rect &rt);

namespace sdl2 {
template <typename Creator, typename Destructor, typename... Arguments>
auto makeResource(Creator c, Destructor d, Arguments &&... args) {
    auto r = c(std::forward<Arguments>(args)...);
    if (!r) {
        throw std::system_error(errno, std::generic_category());
    }
    return std::unique_ptr<std::decay_t<decltype(*r)>, decltype(d)>(r, d);
}

using WindowPtr = std::unique_ptr<SDL_Window, decltype(&SDL_DestroyWindow)>;
using RendererPtr = std::unique_ptr<SDL_Renderer, decltype(&SDL_DestroyRenderer)>;
using TexturePtr = std::unique_ptr<SDL_Texture, decltype(&SDL_DestroyTexture)>;
using SurfacePtr = std::unique_ptr<SDL_Surface, decltype(&SDL_FreeSurface)>;

inline WindowPtr makeWindow(const char *title, int x, int y, int w, int h, Uint32 flags) {
    return makeResource(SDL_CreateWindow, SDL_DestroyWindow, title, x, y, w, h, flags);
}

inline RendererPtr makeRenderer(SDL_Window *window, int index, Uint32 flags) {
    return makeResource(SDL_CreateRenderer, SDL_DestroyRenderer, window, index, flags);
}

inline TexturePtr makeTexture(SDL_Renderer *renderer, Uint32 format, int access, int w, int h) {
    return makeResource(SDL_CreateTexture, SDL_DestroyTexture, renderer, format, access, w, h);
}

inline TexturePtr makeTextureFromSurfaceSection(SDL_Renderer *rdr, SDL_Surface *sf, const SDL_Rect &rt) {
    return makeResource(textureFromSurfaceSection, SDL_DestroyTexture, rdr, sf, rt);
}

inline SurfacePtr makeSurface(Uint32 flags, int width, int height, int depth, Uint32 Rmask, Uint32 Gmask, Uint32 Bmask,
                                  Uint32 Amask) {
    return makeResource(SDL_CreateRGBSurface, SDL_FreeSurface, flags, width, height, depth, Rmask, Gmask, Bmask, Amask);
}

inline SurfacePtr loadSurface(const char *file) { return makeResource(IMG_Load, SDL_FreeSurface, file); }
} // namespace sdl2

void drawRoundedRectangle(SDL_Renderer *s, int r, SDL_Rect *rect, SDL_Color col);
void drawCircle(SDL_Renderer *s, int cx, int cy, int r, SDL_Color col, bool fl);
void addCircleSymmetryPoints(std::vector<SDL_Point> ps, int cx, int cy, int x, int y);
Uint32 getAt(SDL_Surface *s, int x, int y);
void setAt(SDL_Surface *s, int x, int y, Uint32 color);

#endif
