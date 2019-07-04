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

#ifndef SELECTBUTTON_H
#define SELECTBUTTON_H

#include "focusable.hpp"
#include "menubutton.hpp"
#include "scrollbox.hpp"

class SelectButton : public ScrollBox, public MenuButton {
  public:
    SelectButton(SDL_Rect rt, const std::vector<std::string> &is, SDL_Color fg, SDL_Color bg, SDL_Color hl, int b, int r,
                 int fS, std::function<void()> oC);
    const std::string &getItem() const {
        if (highlightLine > -1)
            return items[static_cast<size_t>(highlightLine)];
        return items.back();
    }
    bool keyCaptured(const SDL_KeyboardEvent &k) const;
    void handleKey(const SDL_KeyboardEvent &k);
    void handleTextInput(const SDL_TextInputEvent &) {}
    void handleClick(const SDL_MouseButtonEvent &b);
};
#endif
