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

#include "textbox.hpp"

TextBox::TextBox(const BoxInfo &bI, Printer &pr)
    : rect(bI.rect), fixedSize(rect.w && rect.h), text(bI.text), colors(bI.colors), id(bI.id), size(bI.size),
      behavior(bI.behavior), images(bI.images), printer(pr) {
    setText();
}

void TextBox::setText() {
    // Renders the text using the printer. Call any time text changes.
    if (!fixedSize) {
        // Get rid of old width and height and move rectangle to center.
        rect.x += rect.w / 2;
        rect.y += rect.h / 2;
        rect.w = 0;
        rect.h = 0;
    }
    if (id.second)
        printer.setNationId(id.first);
    else
        printer.setNationId(0);
    printer.setColors(colors);
    printer.setSize(size);
    lineHeight = printer.getFontHeight();
    lines = text.size();
    if (rect.h && lines > size_t(rect.h / lineHeight)) {
        // Truncate lines of text if they won'tx fit in box of fixed size.
        lines = static_cast<size_t>(rect.h / lineHeight);
        text = std::vector<std::string>(begin(text), begin(text) + static_cast<std::vector<std::string>::difference_type>(lines));
    }
    surface = printer.print(text, rect, images);
    updateTexture = true;

    if (!fixedSize) {
        // Rectagle coordinates are at center, move them to top-left corner.
        rect.x -= rect.w / 2;
        rect.y -= rect.h / 2;
    }
}

void TextBox::setBorder(int bd) {
    rect.x += (size.border - bd);
    rect.y += (size.border - bd);
    rect.w += (bd - size.border) * 2;
    rect.h += (bd - size.border) * 2;
    size.border = bd;
    setText();
}

void TextBox::setInvColors(bool i) {
    colors.invert = i;
    setText();
}

void TextBox::setClicked(bool c) {
    clicked = c;
    setInvColors(c);
}

void TextBox::toggleFocus() {
    isFocus = !isFocus;
    if (isFocus) {
        setBorder(size.border * 2);
        if (behavior == BoxInfo::edit) SDL_StartTextInput();
    } else {
        colors.invert = clicked;
        setBorder(size.border / 2);
        if (behavior == BoxInfo::edit) SDL_StopTextInput();
    }
}

void TextBox::place(int x, int y, std::vector<SDL_Rect> &drawn) {
    rect.x = x - rect.w / 2;
    rect.y = y - rect.h / 2;
    SDL_Rect r = rect;
    bool c = true;
    while (c) {
        c = false;
        for (auto &d : drawn)
            if (SDL_HasIntersection(&d, &r)) {
                c = true;
                break;
            }
        if (c) r.y -= r.h / 2;
    }
    rect = r;
    drawn.push_back(r);
}

void TextBox::move(int dx, int dy) {
    rect.x += dx;
    rect.y += dy;
}

void TextBox::draw(SDL_Renderer *s) {
    // Copy this TextBox's texture onto s, updating texture if necessary.
    if (updateTexture) {
        texture.reset(SDL_CreateTextureFromSurface(s, surface.get()));
        updateTexture = false;
    }
    SDL_RenderCopy(s, texture.get(), nullptr, &rect);
}

bool TextBox::keyCaptured(const SDL_KeyboardEvent &k) const {
    switch (k.keysym.sym) {
    case SDLK_ESCAPE:
    case SDLK_TAB:
        return false;
    default:
        return isFocus;
    }
}

bool TextBox::clickCaptured(const SDL_MouseButtonEvent &b) const {
    SDL_Point mp;
    mp.x = b.x;
    mp.y = b.y;
    return (canFocus() && SDL_PointInRect(&mp, &rect));
}

void TextBox::handleKey(const SDL_KeyboardEvent &k) {
    if (k.state == SDL_PRESSED) switch (k.keysym.sym) {
        case SDLK_BACKSPACE:
            if (!text.empty() && !text.back().empty()) text.back().pop_back();
            setText();
        default:
            return;
        }
}

void TextBox::handleTextInput(const SDL_TextInputEvent &tx) {
    if (text.empty()) text.push_back("");
    text.back().append(tx.text);
    setText();
}
