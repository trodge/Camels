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

SelectButton::SelectButton(const BoxInfo &bI, Printer &pr) : TextBox(bI, pr), ScrollBox(bI, pr), MenuButton(bI, pr) {}

bool SelectButton::keyCaptured(const SDL_KeyboardEvent &k) const {
    return MenuButton::keyCaptured(k) || ScrollBox::keyCaptured(k);
}

void SelectButton::handleKey(const SDL_KeyboardEvent &k) {
    ScrollBox::handleKey(k);
    MenuButton::handleKey(k);
}

void SelectButton::handleClick(const SDL_MouseButtonEvent &b) {
    ScrollBox::handleClick(b);
    MenuButton::handleClick(b);
    if (highlightLine == -1) setInvColors(false);
}
