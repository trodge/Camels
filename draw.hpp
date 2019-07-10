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

namespace sdl {

struct Deleter {
    void operator()(SDL_Window *wndw) { SDL_DestroyWindow(wndw); }
    void operator()(SDL_Renderer *rndr) { SDL_DestroyRenderer(rndr); }
    void operator()(SDL_Surface *srfc) { SDL_FreeSurface(srfc); }
    void operator()(SDL_Texture *txtr) { SDL_DestroyTexture(txtr); }
    void operator()(TTF_Font *font) { TTF_CloseFont(font); }
};

template <typename Creator, typename... Arguments> auto makeResource(Creator c, Arguments &&... args) {
    auto r = c(std::forward<Arguments>(args)...);
    if (!r) {
        throw std::system_error(errno, std::generic_category());
    }
    return std::unique_ptr<std::decay_t<decltype(*r)>, Deleter>(r);
}

using WindowPtr = std::unique_ptr<SDL_Window, Deleter>;
using RendererPtr = std::unique_ptr<SDL_Renderer, Deleter>;
using SurfacePtr = std::unique_ptr<SDL_Surface, Deleter>;
using TexturePtr = std::unique_ptr<SDL_Texture, Deleter>;
using FontPtr = std::unique_ptr<TTF_Font, Deleter>;

inline WindowPtr makeWindow(const char *title, int x, int y, int w, int h, Uint32 flags) {
    return makeResource(SDL_CreateWindow, title, x, y, w, h, flags);
}

inline RendererPtr makeRenderer(SDL_Window *window, int index, Uint32 flags) {
    return makeResource(SDL_CreateRenderer, window, index, flags);
}

inline RendererPtr makeSoftwareRenderer(SDL_Surface *surface) { return makeResource(SDL_CreateSoftwareRenderer, surface); }

inline SurfacePtr makeSurface(int width, int height) {
    return makeResource(SDL_CreateRGBSurface, 0u, width, height, 32, rmask, gmask, bmask, amask);
}

inline SurfacePtr makeSurfaceWithFormatFrom(void *pixels, int width, int height, int depth, int pitch, Uint32 format) {
    return makeResource(SDL_CreateRGBSurfaceWithFormatFrom, pixels, width, height, depth, pitch, format);
}

inline SurfacePtr loadImage(const char *file) { return makeResource(IMG_Load, file); }

inline TexturePtr makeTexture(SDL_Renderer *renderer, Uint32 format, int access, int w, int h) {
    return makeResource(SDL_CreateTexture, renderer, format, access, w, h);
}

inline TexturePtr makeTextureFromSurface(SDL_Renderer *renderer, SDL_Surface *surface) {
    return makeResource(SDL_CreateTextureFromSurface, renderer, surface);
}

inline TexturePtr makeTextureFromSurfaceSection(SDL_Renderer *rdr, SDL_Surface *sf, const SDL_Rect &rt) {
    SurfacePtr c(
        makeSurfaceWithFormatFrom(static_cast<Uint8 *>(sf->pixels) + rt.y * sf->pitch + rt.x * sf->format->BytesPerPixel,
                                  rt.w, rt.h, 32, sf->pitch, SDL_PIXELFORMAT_RGB24));
    return makeTextureFromSurface(rdr, c.get());
}

inline FontPtr openFont(const char *file, int ptsize) { return makeResource(TTF_OpenFont, file, ptsize); }
} // namespace sdl

void drawRoundedRectangle(SDL_Renderer *s, int r, SDL_Rect *rect, SDL_Color col);
void drawCircle(SDL_Renderer *s, int cx, int cy, int r, SDL_Color col, bool fl);
void addCircleSymmetryPoints(std::vector<SDL_Point> ps, int cx, int cy, int x, int y);
Uint32 getAt(SDL_Surface *s, int x, int y);
void setAt(SDL_Surface *s, int x, int y, Uint32 color);

#endif
