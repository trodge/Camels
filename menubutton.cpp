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

MenuButton::MenuButton(const SDL_Rect &rt, const std::vector<std::string> &t, const SDL_Color &fg, const SDL_Color &bg,
                       unsigned int i, bool iN, int b, int r, int fS, SDL_Surface *img, const std::function<void()> &oC)
    : TextBox(rt, t, fg, bg, i, iN, b, r, fS, img), onClick(oC) {
    canFocus = true;
}

MenuButton::~MenuButton() {}

bool MenuButton::keyCaptured(const SDL_KeyboardEvent &k) const {
    switch (k.keysym.sym) {
    case SDLK_SPACE:
    case SDLK_RETURN:
    case SDLK_KP_ENTER:
        return true;
    default:
        return false;
    }
}

void MenuButton::activate(Uint8 state) {
    if (state == SDL_PRESSED)
        setInvColors(not clicked);
    else if (invColors != clicked) {
        clicked = invColors;
        onClick();
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
