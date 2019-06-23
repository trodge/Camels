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


void draw_rounded_rectangle(SDL_Surface *s, int r, SDL_Rect rect, SDL_Color col) {
    // Draw a rounded rectangle where corners are circles with radius r on s.
    SDL_FillRect()
}


void draw_circle(SDL_Surface *s, int cx, int cy, int r, SDL_Color col, bool fl) {
    // Draw a circle on s at position (cx, cy) with radius r and color col, either filled or not filled.
    Uint32 color = SDL_MapRGB(s->format, col.r, col.g, col.b);
    if (fl) {
        if (cx - r >= 0 and cx + r < s->w and cy - r >= 0 and cy + r < s->h) {
            // circle is in bounds so skip per-pixel bounds check
            for (int x = -r; x <= r; ++x)
                for (int y = -r; y <= r; ++y)
                    if ((x * x) + (y * y) <= r * r)
                        draw_pixel(s, cx + x, cy + y, color);
        } else if (cx + r >= 0 and cx - r < s->w and cy + r >= 0 and cy - r < s->h)
            // circle is partially in bounds
            for (int x = -r; x <= r; ++x)
                for (int y = -r; y <= r; ++y)
                    if ((x * x) + (y * y) <= r * r and cx + x >= 0 and cx + x < s->w and cy + y >= 0 and cy + y < s->h)
                        draw_pixel(s, cx + x, cy + y, color);
    } else {
        int x = 0;
        int y = r;
        int d = 1 - r;
        if (cx - r >= 0 and cx + r < s->w and cy - r >= 0 and cy + r < s->h) {
            draw_circle_symmetry_points(s, cx, cy, x, y, color, false);
            while (x < y) {
                ++x;
                if (d < 0) {
                    d += (x << 1) + 1;
                } else {
                    --y;
                    d += ((x - y) << 1) + 1;
                }
                draw_circle_symmetry_points(s, cx, cy, x, y, color, false);
            }
        } else if (cx + r >= 0 and cx - r < s->w and cy + r >= 0 and cy - r < s->h) {
            // circle is partially on surface, must check bounds
            draw_circle_symmetry_points(s, cx, cy, x, y, color, true);
            while (x < y) {
                ++x;
                if (d < 0) {
                    d += (x << 1) + 1;
                } else {
                    --y;
                    d += ((x - y) << 1) + 1;
                }
                draw_circle_symmetry_points(s, cx, cy, x, y, color, true);
            }
        }
    }
}

void draw_circle_symmetry_points(SDL_Surface *s, int cx, int cy, int x, int y, Uint32 color, bool check_bounds) {
    if (check_bounds) {
        if (cx + x >= 0 and cx + x < s->w and cy + y >= 0 and cy + y < s->h) {
            draw_pixel(s, (cx + x), (cy + y), color);
        }
        if (cx + x >= 0 and cx + x < s->w and cy - y >= 0 and cy - y < s->h) {
            draw_pixel(s, (cx + x), (cy - y), color);
        }
        if (cx - x >= 0 and cx - x < s->w and cy + y >= 0 and cy + y < s->h) {
            draw_pixel(s, (cx - x), (cy + y), color);
        }
        if (cx - x >= 0 and cx - x < s->w and cy - y >= 0 and cy - y < s->h) {
            draw_pixel(s, (cx - x), (cy - y), color);
        }
        if (cx + y >= 0 and cx + y < s->w and cy + x >= 0 and cy + x < s->h) {
            draw_pixel(s, (cx + y), (cy + x), color);
        }
        if (cx + y >= 0 and cx + y < s->w and cy - x >= 0 and cy - x < s->h) {
            draw_pixel(s, (cx + y), (cy - x), color);
        }
        if (cx - y >= 0 and cx - y < s->w and cy + x >= 0 and cy + x < s->h) {
            draw_pixel(s, (cx - y), (cy + x), color);
        }
        if (cx - y >= 0 and cx - y < s->w and cy - x >= 0 and cy - x < s->h) {
            draw_pixel(s, (cx - y), (cy - x), color);
        }
    } else {
        draw_pixel(s, (cx + x), (cy + y), color);
        draw_pixel(s, (cx + x), (cy - y), color);
        draw_pixel(s, (cx - x), (cy + y), color);
        draw_pixel(s, (cx - x), (cy - y), color);
        draw_pixel(s, (cx + y), (cy + x), color);
        draw_pixel(s, (cx + y), (cy - x), color);
        draw_pixel(s, (cx - y), (cy + x), color);
        draw_pixel(s, (cx - y), (cy - x), color);
    }
}

void draw_line(SDL_Surface *s, int xi, int yi, int xf, int yf, SDL_Color col) {
    // Draw a line on surface s from (xi, yi) to (xf, yf) in color col.
    // Check if line is trivially out of bounds
    int w = s->w - 1;
    int h = s->h - 1;
    if ((xi < 0 and xf < 0) or (xi > w and xf > h) or (yi < 0 and yf < 0) or (yi > h and yf > h))
        return;
    if (xi < 0 or xi > w or yi < 0 or yi > h or xf < 0 or xf > w or yf < 0 or yf > h) {
        // Clip line
        double dx = xf - xi, dy = yf - yi, ti = 0, tf = 1;
        for (int side = 0; side < 4; ++side) {
            double p, q, r;
            switch (side) {
            case 0:
                p = -dx;
                q = xi;
                break;
            case 1:
                p = dx;
                q = w - xi;
                break;
            case 2:
                p = -dy;
                q = yi;
                break;
            case 3:
                p = dy;
                q = h - yi;
                break;
            }
            r = q / p;
            if (p < 0) {
                if (r > tf)
                    return;
                else if (r > ti)
                    ti = r;
            } else if (p > 0) {
                if (r < ti)
                    return;
                else if (r < tf)
                    tf = r;
            }
        }
        xf = xi + tf * dx;
        yf = yi + tf * dy;
        xi += ti * dx;
        yi += ti * dy;
    }
    // Draw the line.
    int dx = xf - xi;
    int dy = yf - yi;
    Uint32 color = SDL_MapRGB(s->format, col.r, col.g, col.b);
    bool steep = abs(dy) > abs(dx);
    if (steep) {
        // switch x and y
        std::swap(xi, yi);
        std::swap(xf, yf);
        std::swap(dx, dy);
    }
    bool left = dx < 0;
    if (left) {
        // switch i and f
        std::swap(xi, xf);
        std::swap(yi, yf);
        dx = -dx;
    }
    int e = -dx;
    int dey = 2 * abs(dy);
    int dex = 2 * dx;
    int ystep;
    int y = yi;
    if (yi < yf)
        ystep = 1;
    else
        ystep = -1;
    int x;
    for (x = xi; x <= xf; x++) {
        if (steep) {
            // if (x > -1 and x < s->h and y > -1 and y < s->w)
            draw_pixel(s, y, x, color);
            // else std::cout << "Drawing off screen: (" << y << ',' << x << ")
            // (" <<
            // yi << ',' << xi << ") -> (" << yf << ',' << xf << //')'  << steep
            // <<
            // std::endl;
        } else {
            // if (x > -1 and x < s->w and y > -1 and y < s->h)
            draw_pixel(s, x, y, color);
            // else std::cout << "Drawing off screen: (" << x << ',' << y << ")
            // (" <<
            // xi << ',' << yi << ") -> (" << xf << ',' << yf << //')' << steep
            // <<
            // std::endl;
        }
        e += dey;
        if (e > 0) {
            y += ystep;
            e -= dex;
        }
    }
}

Uint32 get_at(const SDL_Surface *s, int x, int y) {
    int bpp = s->format->BytesPerPixel;
    Uint8 *p = (Uint8 *)s->pixels + y * s->pitch + x * bpp;

    switch (bpp) {
    case 1:
        return *p;
        break;

    case 2:
        return *(Uint16 *)p;
        break;

    case 3:
        if (SDL_BYTEORDER == SDL_BIG_ENDIAN)
            return p[0] << 16 | p[1] << 8 | p[2];
        else
            return p[0] | p[1] << 8 | p[2] << 16;
        break;

    case 4:
        return *(Uint32 *)p;
        break;

    default:
        return 0;
    }
}

void draw_pixel(SDL_Surface *s, int x, int y, Uint32 color) {
    int bpp = s->format->BytesPerPixel;
    Uint8 *p = (Uint8 *)s->pixels + y * s->pitch + x * bpp;

    switch (bpp) {
    case 1:
        *p = color;
        break;

    case 2:
        *(Uint16 *)p = color;
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
        *(Uint32 *)p = color;
        break;

    default:
        break;
    }
}