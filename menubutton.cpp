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

#include "menubutton.hpp"

MenuButton::MenuButton(const BoxInfo &bI, Printer &pr) : TextBox(bI, pr), key(bI.key), onClick(bI.onClick) {
    canFocus = true;
}

void MenuButton::toggleFocus(bool iTn) {
    if (isFocus) setInvColors(clicked);
    TextBox::toggleFocus(iTn);
}

bool MenuButton::keyCaptured(const SDL_KeyboardEvent &k) const {
    switch (k.keysym.sym) {
    case SDLK_SPACE:
    case SDLK_RETURN:
    case SDLK_KP_ENTER:
        return isFocus;
    default:
        return k.keysym.sym == key;
    }
}

void MenuButton::activate(Uint8 state) {
    if (state == SDL_PRESSED)
        setInvColors(!clicked);
    else if (invColors != clicked) {
        clicked = invColors;
        onClick(this);
    }
}

void MenuButton::handleKey(const SDL_KeyboardEvent &k) {
    switch (k.keysym.sym) {
    case SDLK_SPACE:
    case SDLK_RETURN:
    case SDLK_KP_ENTER:
        activate(k.state);
        return;
    default:
        if (k.keysym.sym == key)
            activate(k.state);
        else
            setInvColors(clicked);
    }
}

void MenuButton::handleClick(const SDL_MouseButtonEvent &b) { activate(b.state); }
