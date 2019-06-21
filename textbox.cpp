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

#include "textbox.h"

TextBox::TextBox(SDL_Rect r, const std::vector<std::string> &t, SDL_Color fg, SDL_Color bg, int i, bool iN, int b, int fS)
    : rect(r), text(t), foreground(fg), background(bg), id(i), isNation(iN), border(b), fontSize(fS) {
    fixedSize = (r.w and r.h);
    setText(t);
}

TextBox::TextBox(SDL_Rect r, const std::vector<std::string> &t, SDL_Color fg, SDL_Color bg, int i, int b, int fS)
    : TextBox(r, t, fg, bg, i, false, b, fS) {}

TextBox::TextBox(SDL_Rect r, const std::vector<std::string> &t, SDL_Color fg, SDL_Color bg, int b, int fS)
    : TextBox(r, t, fg, bg, 0, b, fS) {}

TextBox::~TextBox() { SDL_FreeSurface(ts); }

void TextBox::setText() {
    // Renders the text using the printer. Call any time text changes.
    if (not fixedSize) {
        rect.x += rect.w / 2;
        rect.y += rect.h / 2;
        rect.w = 0;
        rect.h = 0;
    }
    if (ts)
        SDL_FreeSurface(ts);
    if (isNation)
        Printer::setNationId(id);
    else
        Printer::setNationId(0);
    if (invColors)
        Printer::setColors(background, foreground);
    else
        Printer::setColors(foreground, background);
    Printer::setSize(fontSize);
    lineHeight = Printer::getFontHeight();
    lines = text.size();
    if (rect.h and lines > size_t(rect.h / lineHeight)) {
        // Truncate lines of text if they won't fit in box of fixed size.
        lines = rect.h / lineHeight;
        text = std::vector<std::string>(text.begin(), text.begin() + lines);
    }
    ts = Printer::print(text, rect.w, rect.h, border);
    // If rectangle dimensions were not provided, rectagle coordinates are text center.
    if (not fixedSize) {
        rect.w = ts->w;
        rect.h = ts->h;
        rect.x -= rect.w / 2;
        rect.y -= rect.h / 2;
    }
}

void TextBox::setBorder(int b) {
    rect.x += (border - b);
    rect.y += (border - b);
    rect.w += (b - border) * 2;
    rect.h += (b - border) * 2;
    border = b;
    setText();
}

void TextBox::setInvColors(bool i) {
    invColors = i;
    setText();
}

void TextBox::setClicked(bool c) {
    clicked = c;
    setInvColors(c);
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
        if (c)
            r.y -= r.h / 2;
    }
    rect = r;
    drawn.push_back(r);
}

bool TextBox::keyCaptured(const SDL_KeyboardEvent &k) const {
    switch (k.keysym.sym) {
    case SDLK_ESCAPE:
    case SDLK_TAB:
        return false;
    default:
        return true;
    }
}

bool TextBox::clickCaptured(const SDL_MouseButtonEvent &b) const {
    SDL_Point mp;
    mp.x = b.x;
    mp.y = b.y;
    return (SDL_PointInRect(&mp, &rect));
}

void TextBox::handleKey(const SDL_KeyboardEvent &k) {
    if (k.state == SDL_PRESSED)
        switch (k.keysym.sym) {
        case SDLK_BACKSPACE:
            if (not text.empty() and not text.back().empty())
                text.back().pop_back();
            setText();
        default:
            return;
        }
}

void TextBox::handleTextInput(const SDL_TextInputEvent &t) {
    if (not text.empty()) {
        text.back().append(t.text);
        setText();
    }
}
