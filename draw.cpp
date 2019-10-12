/*
 * This file is part of Camels.
 *
 * Camels is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at p.your option) any later version.
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

#include "draw.hpp"

void Position::place(const SDL_Point &ofs, double s) {
    point.x = int(s * longitude) + ofs.x;
    point.y = ofs.y - int(s * latitude);
}

int Position::distSq(const Position &pos) const {
    SDL_Point dist = {pos.point.x - point.x, pos.point.y - point.y};
    return (dist.x) * (dist.x) + (dist.y) * (dist.y);
}

bool Position::stepToward(const Position &pos, double tm) {
    // Take a step toward given position for given time. Return false if position reached otherwise return true.
    double dlt = pos.latitude - latitude;
    double dlg = pos.longitude - longitude;
    double ds = dlt * dlt + dlg * dlg;
    if (ds > tm * tm) {
        // There remains more distance to travel.
        if (dlg != 0) {
            double m = dlt / dlg;
            double dxs = tm * tm / (1 + m * m);
            double dys = tm * tm - dxs;
            latitude += copysign(sqrt(dys), dlt);
            longitude += copysign(sqrt(dxs), dlg);
        } else
            latitude += copysign(tm, dlt);
        return true;
    }
    // We have reached the destination.
    latitude = pos.latitude;
    longitude = pos.longitude;
    return false;
}

void drawRoundedRectangle(SDL_Renderer *s, int r, SDL_Rect *rect, SDL_Color col) {
    // Draw a rounded rectangle where corners are circles with radius r on s.
    // Fill top, middle, bottom rectangles
    std::array<SDL_Rect, 3> rects = {{{rect->x + r, rect->y, rect->w - 2 * r, r},
                                      {rect->x, rect->y + r, rect->w, rect->h - 2 * r},
                                      {rect->x + r, rect->y + rect->h - r, rect->w - 2 * r, r}}};
    SDL_SetRenderDrawColor(s, col.r, col.g, col.b, col.a);
    SDL_RenderFillRects(s, rects.data(), 3);
    std::vector<SDL_Point> ps; // points to draw partial circles
    ps.reserve(static_cast<size_t>(kPi * r * r + 1));
    // Fill partial circle in top-left corner.
    for (int x = -r; x <= 0; ++x)
        for (int y = -r; y <= 0; ++y)
            if ((x * x) + (y * y) <= r * r) ps.push_back({rect->x + r + x, rect->y + r + y});
    // Fill partial circle in top-right corner.
    for (int x = 0; x <= r; ++x)
        for (int y = -r; y <= 0; ++y)
            if ((x * x) + (y * y) <= r * r) ps.push_back({rect->x + rect->w - 1 - r + x, rect->y + r + y});
    // Fill partial circle in bottom-left corner.
    for (int x = -r; x <= 0; ++x)
        for (int y = 0; y <= r; ++y)
            if ((x * x) + (y * y) <= r * r) ps.push_back({rect->x + r + x, rect->y + rect->h - 1 - r + y});
    // Fill partial circle in bottom-right corner.
    for (int x = 0; x <= r; ++x)
        for (int y = 0; y <= r; ++y)
            if ((x * x) + (y * y) <= r * r)
                ps.push_back({rect->x + rect->w - 1 - r + x, rect->y + rect->h - 1 - r + y});
    SDL_RenderDrawPoints(s, ps.data(), static_cast<int>(ps.size()));
}

void drawCircle(SDL_Renderer *s, const SDL_Point &c, int r, SDL_Color col, bool fl) {
    // Draw a circle on s with center c, radius r and color col, either filled or not filled.
    std::vector<SDL_Point> points;
    if (fl) {
        // filled circle
        points.reserve(static_cast<size_t>(kPi * r * r + 1));
        for (int x = -r; x <= r; ++x)
            for (int y = -r; y <= r; ++y)
                if ((x * x) + (y * y) <= r * r) points.push_back({c.x + x, c.y + y});
    } else {
        points.reserve(static_cast<size_t>(kPi * r * 2 + 1));
        int x = 0;
        int y = r;
        int d = 1 - r;
        auto symmetryPoints = circleSymmetryPoints(c, {x, y});
        points.insert(end(points), begin(symmetryPoints), end(symmetryPoints));
        while (x < y) {
            ++x;
            if (d < 0) {
                d += (x << 1) + 1;
            } else {
                --y;
                d += ((x - y) << 1) + 1;
            }
            symmetryPoints = circleSymmetryPoints(c, {x, y});
            points.insert(end(points), begin(symmetryPoints), end(symmetryPoints));
        }
    }
    SDL_SetRenderDrawColor(s, col.r, col.g, col.b, col.a);
    SDL_RenderDrawPoints(s, points.data(), static_cast<int>(points.size()));
}

std::array<SDL_Point, 8> circleSymmetryPoints(const SDL_Point &c, const SDL_Point &p) {
    return {{{(c.x + p.x), (c.y + p.y)},
             {(c.x + p.x), (c.y - p.y)},
             {(c.x - p.x), (c.y + p.y)},
             {(c.x - p.x), (c.y - p.y)},
             {(c.x + p.y), (c.y + p.x)},
             {(c.x + p.y), (c.y - p.x)},
             {(c.x - p.y), (c.y + p.x)},
             {(c.x - p.y), (c.y - p.x)}}};
}

Uint32 getAt(SDL_Surface *s, const SDL_Point &pt) {
    // Get color of pixel in surface s at (x, p.y)
    SDL_LockSurface(s);
    int bpp = s->format->BytesPerPixel;
    Uint8 *pix = static_cast<Uint8 *>(s->pixels) + pt.y * s->pitch + pt.x * bpp;
    SDL_UnlockSurface(s);

    switch (bpp) {
    case 1:
        return *pix;
        break;
    case 2:
        return *reinterpret_cast<Uint16 *>(pix);
        break;
    case 3:
        if (SDL_BYTEORDER == SDL_BIG_ENDIAN)
            return static_cast<Uint32>(pix[0] << 16 | pix[1] << 8 | pix[2]);
        else
            return static_cast<Uint32>(pix[0] | pix[1] << 8 | pix[2] << 16);
        break;
    case 4:
        return *reinterpret_cast<Uint32 *>(pix);
        break;
    default:
        return 0;
    }
}

void setAt(SDL_Surface *s, const SDL_Point &pt, Uint32 color) {
    // Set pixel in surface s at (x, p.y) to color
    SDL_LockSurface(s);
    int bpp = s->format->BytesPerPixel;
    Uint8 *pix = static_cast<Uint8 *>(s->pixels) + pt.y * s->pitch + pt.x * bpp;

    switch (bpp) {
    case 1:
        *pix = static_cast<Uint8>(color);
        break;

    case 2:
        *reinterpret_cast<Uint16 *>(pix) = static_cast<Uint16>(color);
        break;

    case 3:
        if (SDL_BYTEORDER == SDL_BIG_ENDIAN) {
            pix[0] = (color >> 16) & 0xff;
            pix[1] = (color >> 8) & 0xff;
            pix[2] = color & 0xff;
        } else {
            pix[0] = color & 0xff;
            pix[1] = (color >> 8) & 0xff;
            pix[2] = (color >> 16) & 0xff;
        }
        break;

    case 4:
        *reinterpret_cast<Uint32 *>(pix) = static_cast<Uint32>(color);
        break;

    default:
        break;
    }
    SDL_UnlockSurface(s);
}
