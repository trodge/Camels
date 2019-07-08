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

#include "selectbutton.hpp"

SelectButton::SelectButton(const SDL_Rect &rt, const std::vector<std::string> &is, const SDL_Color &fg, const SDL_Color &bg, const SDL_Color &hl, int b,
                           int r, int fS, const std::function<void()> &oC)
    : TextBox(rt, is, fg, bg, 0, false, b, r, fS), ScrollBox(is, hl), MenuButton(oC) {
    canFocus = true;
}

bool SelectButton::keyCaptured(const SDL_KeyboardEvent &k) const {
    return MenuButton::keyCaptured(k) or ScrollBox::keyCaptured(k);
}

void SelectButton::handleKey(const SDL_KeyboardEvent &k) {
    ScrollBox::handleKey(k);
    MenuButton::handleKey(k);
}

void SelectButton::handleClick(const SDL_MouseButtonEvent &b) {
    ScrollBox::handleClick(b);
    MenuButton::handleClick(b);
    if (highlightLine == -1)
        setInvColors(false);
}
