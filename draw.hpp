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

#include "constants.hpp"

class Position {
    double longitude, latitude;
    SDL_Point point;

public:
    Position(double lng, double ltt) : longitude(lng), latitude(ltt) {}
    double getLongitude() const { return longitude; }
    double getLatitude() const { return latitude; }
    const SDL_Point &getPoint() const { return point; }
    int distSq(const Position &pos) const;
    void setLongitude(double lng) { longitude = lng; }
    void setLatitude(double ltt) { latitude = ltt; }
    void place(const SDL_Point &ofs, double s);
    bool stepToward(const Position &pos, double tm);
};

/* SDL interprets each pixel as a 32-bit number, so our masks must depend
   on the endianness (byte order) of the machine */
const Uint32 surfaceFlags = 0;

const int bitDepth = 32;

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

    template <typename T> using Handle = std::unique_ptr<T, Deleter>;

    using Window = Handle<SDL_Window>;
    using Renderer = Handle<SDL_Renderer>;
    using Surface = Handle<SDL_Surface>;
    using Texture = Handle<SDL_Texture>;
    using Font = Handle<TTF_Font>;

    inline Texture textureFromSurfaceSection(SDL_Renderer *rdr, SDL_Surface *sf, const SDL_Rect &rt) {
        Surface c(SDL_CreateRGBSurfaceWithFormatFrom(
            static_cast<Uint8 *>(sf->pixels) + rt.y * sf->pitch + rt.x * sf->format->BytesPerPixel, rt.w, rt.h,
            32, sf->pitch, SDL_PIXELFORMAT_RGB24));
        return Texture(SDL_CreateTextureFromSurface(rdr, c.get()));
    }
    
    Font openFont(std::string fN, int fS);
} // namespace sdl

void drawRoundedRectangle(SDL_Renderer *s, int r, SDL_Rect *rect, SDL_Color col);
void drawCircle(SDL_Renderer *s, const SDL_Point &c, int r, SDL_Color col, bool fl);
std::array<SDL_Point, 8> circleSymmetryPoints(const SDL_Point &c, const SDL_Point &p);
Uint32 getAt(SDL_Surface *s, const SDL_Point &pt);
void setAt(SDL_Surface *s, const SDL_Point &pt, Uint32 color);

#endif
