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

#include "scrollbox.h"

ScrollBox::ScrollBox(SDL_Rect r, const std::vector<std::string> &is, SDL_Color fg, SDL_Color bg, SDL_Color hl, int b, int fS)
    : TextBox(r, is, fg, bg, 0, false, b, fS), highlight(hl), items(is) {
    canFocus = true;
}

ScrollBox::~ScrollBox() {}

void ScrollBox::setText() {
    Printer::setHighlightLine(highlightLine - scroll);
    Printer::setHighlight(highlight);
    TextBox::setText();
    Printer::setHighlightLine(-1);
}

void ScrollBox::setItems(const std::vector<std::string> &is) {
    items = is;
    scroll = 0;
    setHighlightLine(-1);
    setText();
}

void ScrollBox::addItem(const std::string &i) {
    Printer::setSize(fontSize);
    std::string entry = i, overflow;
    while (Printer::getFontWidth(entry) > rect.w) {
        overflow.insert(overflow.begin(), entry.back());
        entry.pop_back();
    }
    items.push_back(entry);
    setText();
    if (not overflow.empty())
        addItem(overflow);
}

void ScrollBox::setHighlightLine(int h) {
    // Set highlight line to h, count from last line if negative
    if (not lines)
        return;
    while (h < 0)
        h += items.size();
    highlightLine = h;
    if (h) {
        if (size_t(h) >= scroll + lines) {
            scroll = h - lines + 1;
            if (scroll + lines > items.size())
                scroll = items.size() - lines;
        } else if (size_t(h) <= scroll)
            scroll = h - 1;
    } else
        scroll = 0;
    clicked = false;
    invColors = false;
    setText(std::vector<std::string>(items.begin() + scroll, items.begin() + scroll + lines));
}

bool ScrollBox::keyCaptured(const SDL_KeyboardEvent &k) const {
    switch (k.keysym.sym) {
    case SDLK_UP:
    case SDLK_DOWN:
        return true;
    default:
        return false;
    }
}

void ScrollBox::handleKey(const SDL_KeyboardEvent &k) {
    if (k.state == SDL_PRESSED)
        switch (k.keysym.sym) {
        case SDLK_UP:
            if (highlightLine > 0)
                setHighlightLine(highlightLine - 1);
            break;
        case SDLK_DOWN:
            if (highlightLine < int(items.size() - 1))
                setHighlightLine(highlightLine + 1);
            break;
        default:
            break;
        }
}

void ScrollBox::handleClick(const SDL_MouseButtonEvent &b) {
    int y = b.y - rect.y - rect.h / 2 + lines * lineHeight / 2;
    int h = -1;
    if (y >= 0)
        h = y / lineHeight + scroll;
    if (h != highlightLine and h > -1 and size_t(h) < items.size())
        setHighlightLine(h);
}