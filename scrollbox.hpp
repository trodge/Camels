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

#ifndef SCROLLBOX_H
#define SCROLLBOX_H

#include "textbox.hpp"

class ScrollBox : virtual public TextBox {
  protected:
    int highlightLine = -1;
    size_t scroll = 0;
    SDL_Color highlight = {0, 0, 0, 0};
    std::vector<std::string> items;

  public:
    ScrollBox(const BoxInfo &bI, Printer &pr);
    virtual ~ScrollBox() {}
    void setText();
    void setText(const std::vector<std::string> &t) {
        text = t;
        setText();
    }
    void setItems(const std::vector<std::string> &is);
    void addItem(const std::string &i);
    void setHighlightLine(int h);
    int getHighlightLine() const { return highlightLine; }
    virtual bool keyCaptured(const SDL_KeyboardEvent &k) const;
    virtual void handleKey(const SDL_KeyboardEvent &k);
    void handleTextInput(const SDL_TextInputEvent &) {}
    virtual void handleClick(const SDL_MouseButtonEvent &);
};

#endif // SCROLLBOX_H
