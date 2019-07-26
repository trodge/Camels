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

#ifndef MENUBUTTON_H
#define MENUBUTTON_H

#include <functional>

#include "textbox.hpp"

class MenuButton : virtual public TextBox {
    void activate(Uint8 state);

  protected:
    SDL_Keycode key = SDLK_UNKNOWN;
    std::function<void(MenuButton *)> onClick;

  public:
    MenuButton(const BoxInfo &bI, Printer &pr);
    virtual ~MenuButton() {}
    SDL_Keycode getKey() const { return key; }
    virtual bool keyCaptured(const SDL_KeyboardEvent &k) const;
    virtual void handleKey(const SDL_KeyboardEvent &k);
    void handleTextInput(const SDL_TextInputEvent &) {}
    virtual void handleClick(const SDL_MouseButtonEvent &b);
};

#endif // MENUBUTTON_H
