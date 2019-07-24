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

#include "scrollbox.hpp"

ScrollBox::ScrollBox(const BoxInfo &bI, Printer &pr)
    : TextBox(bI, pr), highlight(bI.highlight), items(bI.text) {
    canFocus = true;
}

void ScrollBox::setText() {
    printer.setHighlightLine(highlightLine - static_cast<int>(scroll));
    printer.setHighlight(highlight);
    TextBox::setText();
    printer.setHighlightLine(-1);
}

void ScrollBox::setItems(const std::vector<std::string> &is) {
    items = is;
    scroll = 0;
    setHighlightLine(-1);
    setText();
}

void ScrollBox::addItem(const std::string &i) {
    printer.setSize(fontSize);
    std::string entry = i, overflow;
    while (printer.getFontWidth(entry) > rect.w) {
        overflow.insert(overflow.begin(), entry.back());
        entry.pop_back();
    }
    items.push_back(entry);
    setText();
    if (!overflow.empty())
        addItem(overflow);
}

void ScrollBox::setHighlightLine(int h) {
    // Set highlight line to h, count from last line if negative
    if (!lines)
        return;
    while (h < 0)
        h += static_cast<int>(items.size());
    highlightLine = h;
    if (h) {
        if (static_cast<size_t>(h) >= scroll + lines) {
            scroll = static_cast<size_t>(h) - lines + 1;
            if (scroll + lines > items.size())
                scroll = items.size() - lines;
        } else if (static_cast<size_t>(h) <= scroll)
            scroll = static_cast<size_t>(h) - 1;
    } else
        scroll = 0;
    clicked = false;
    invColors = false;
    setText(
        std::vector<std::string>(items.begin() + static_cast<std::vector<std::string>::difference_type>(scroll),
                                 items.begin() + static_cast<std::vector<std::string>::difference_type>(scroll + lines)));
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
    int y = b.y - rect.y - rect.h / 2 + static_cast<int>(lines) * static_cast<int>(lineHeight) / 2;
    int h = -1;
    if (y >= 0)
        h = y / static_cast<int>(lineHeight) + static_cast<int>(scroll);
    if (h != highlightLine && h > -1 && size_t(h) < items.size())
        setHighlightLine(h);
}
