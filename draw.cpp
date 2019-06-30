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

#include "draw.h"

void drawRoundedRectangle(SDL_Renderer *s, int r, SDL_Rect *rect, SDL_Color col) {
    // Draw a rounded rectangle where corners are circles with radius r on s.
    // Fill top, middle, bottom rectangles
    std::array<SDL_Rect, 3> rects = {{{rect->x + r, rect->y, rect->w - 2 * r, r},
                                      {rect->x, rect->y + r, rect->w, rect->h - 2 * r},
                                      {rect->x + r, rect->y + rect->h - r, rect->w - 2 * r, r}}};
    SDL_SetRenderDrawColor(s, col.r, col.g, col.b, col.a);
    SDL_RenderFillRects(s, rects.data(), 3);
    std::vector<SDL_Point> ps; // points to draw partial circles
    ps.reserve(static_cast<size_t>(M_PI * r * r + 1.));
    // Fill partial circle in top-left corner.
    for (int x = -r; x <= 0; ++x)
        for (int y = -r; y <= 0; ++y)
            if ((x * x) + (y * y) <= r * r)
                ps.push_back({rect->x + r + x, rect->y + r + y});
    // Fill partial circle in top-right corner.
    for (int x = 0; x <= r; ++x)
        for (int y = -r; y <= 0; ++y)
            if ((x * x) + (y * y) <= r * r)
                ps.push_back({rect->x + rect->w - 1 - r + x, rect->y + r + y});
    // Fill partial circle in bottom-left corner.
    for (int x = -r; x <= 0; ++x)
        for (int y = 0; y <= r; ++y)
            if ((x * x) + (y * y) <= r * r)
                ps.push_back({rect->x + r + x, rect->y + rect->h - 1 - r + y});
    // Fill partial circle in bottom-right corner.
    for (int x = 0; x <= r; ++x)
        for (int y = 0; y <= r; ++y)
            if ((x * x) + (y * y) <= r * r)
                ps.push_back({rect->x + rect->w - 1 - r + x, rect->y + rect->h - 1 - r + y});
    SDL_RenderDrawPoints(s, ps.data(), static_cast<int>(ps.size()));
}

void drawCircle(SDL_Renderer *s, int cx, int cy, int r, SDL_Color col, bool fl) {
    // Draw a circle on s at position (cx, cy) with radius r and color col, either filled or not filled.
    std::vector<SDL_Point> ps;
    if (fl) {
        // filled circle
        ps.reserve(static_cast<size_t>(M_PI * r * r + 1.));
        for (int x = -r; x <= r; ++x)
            for (int y = -r; y <= r; ++y)
                if ((x * x) + (y * y) <= r * r)
                    ps.push_back({cx + x, cy + y});
    } else {
        ps.reserve(static_cast<size_t>(M_PI * r * 2. + 1.));
        int x = 0;
        int y = r;
        int d = 1 - r;
        addCircleSymmetryPoints(ps, cx, cy, x, y);
        while (x < y) {
            ++x;
            if (d < 0) {
                d += (x << 1) + 1;
            } else {
                --y;
                d += ((x - y) << 1) + 1;
            }
            addCircleSymmetryPoints(ps, cx, cy, x, y);
        }
    }
    SDL_SetRenderDrawColor(s, col.r, col.g, col.b, col.a);
    SDL_RenderDrawPoints(s, ps.data(), static_cast<int>(ps.size()));
}

void addCircleSymmetryPoints(std::vector<SDL_Point> ps, int cx, int cy, int x, int y) {
    ps.push_back({(cx + x), (cy + y)});
    ps.push_back({(cx + x), (cy - y)});
    ps.push_back({(cx - x), (cy + y)});
    ps.push_back({(cx - x), (cy - y)});
    ps.push_back({(cx + y), (cy + x)});
    ps.push_back({(cx + y), (cy - x)});
    ps.push_back({(cx - y), (cy + x)});
    ps.push_back({(cx - y), (cy - x)});
}

Uint32 getAt(const SDL_Surface *s, int x, int y) {
    // Get color of pixel in surface s at (x, y)
    int bpp = s->format->BytesPerPixel;
    Uint8 *p = static_cast<Uint8 *>(s->pixels) + y * s->pitch + x * bpp;

    switch (bpp) {
    case 1:
        return *p;
        break;

    case 2:
        return *reinterpret_cast<Uint16 *>(p);
        break;

    case 3:
        if (SDL_BYTEORDER == SDL_BIG_ENDIAN)
            return static_cast<Uint32>(p[0] << 16u | p[1] << 8u | p[2]);
        else
            return static_cast<Uint32>(p[0] | p[1] << 8u | p[2] << 16u);
        break;

    case 4:
        return *reinterpret_cast<Uint32 *>(p);
        break;

    default:
        return 0;
    }
}

void setAt(SDL_Surface *s, int x, int y, Uint32 color) {
    // Set pixel in surface s at (x, y) to color
    int bpp = s->format->BytesPerPixel;
    Uint8 *p = static_cast<Uint8 *>(s->pixels) + y * s->pitch + x * bpp;

    switch (bpp) {
    case 1:
        *p = static_cast<Uint8>(color);
        break;

    case 2:
        *reinterpret_cast<Uint16 *>(p) = static_cast<Uint16>(color);
        break;

    case 3:
        if (SDL_BYTEORDER == SDL_BIG_ENDIAN) {
            p[0] = (color >> 16) & 0xff;
            p[1] = (color >> 8) & 0xff;
            p[2] = color & 0xff;
        } else {
            p[0] = color & 0xff;
            p[1] = (color >> 8) & 0xff;
            p[2] = (color >> 16) & 0xff;
        }
        break;

    case 4:
        *reinterpret_cast<Uint32 *>(p) = static_cast<Uint32>(color);
        break;

    default:
        break;
    }
}
