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

#ifndef TEXTBOX_H
#define TEXTBOX_H

#include <iostream>
#include <string>
#include <vector>

#include <SDL2/SDL.h>

#include "draw.hpp"
#include "printer.hpp"
#include "settings.hpp"

class TextBox {
protected:
    SDL_Rect rect{0, 0, 0, 0};
    bool fixedSize = false, isFocus = false;
    std::vector<std::string> text;
    size_t lines = 0;
    ColorScheme colors;
    std::pair<unsigned int, bool> id = {0, false}; // id and whether id is nation id
    BoxSize size;
    BoxBehavior behavior; // inert, focus, edit, scroll
    int lineHeight = -1;  // radius of rounded corner circles
    bool clicked = false; // default button behavoir is to toggle between clicked and unclicked state
    sdl::Surface surface;
    sdl::Texture texture;
    bool updateTexture = false; // whether the texture needs updating to match the surface
    std::vector<Image> images;
    Printer &printer;
    void setBorder(int bd);

public:
    TextBox(const BoxInfo &bI, Printer &pr);
    virtual ~TextBox() {}
    const SDL_Rect &getRect() const { return rect; }
    bool canFocus() const { return behavior != BoxBehavior::inert; }
    const std::vector<std::string> &getText() const { return text; }
    const std::string &getText(size_t i) const { return text[i]; }
    virtual const std::string &getItem() const { return text.back(); }
    bool getClicked() const { return clicked; }
    unsigned int getId() const { return id.first; }
    virtual int getHighlightLine() const { return -1; }
    virtual void setText();
    void setText(const std::vector<std::string> &tx) {
        text = tx;
        setText();
    }
    void setText(size_t i, const std::string &tx) {
        text[i] = tx;
        setText();
    }
    void addText(const std::string &tx) {
        text.push_back(tx);
        setText();
    }
    virtual void addItem(const std::string &i) { addText(i); }
    void setInvColors(bool i);
    void setClicked(bool c);
    virtual void setKey(const SDL_Keycode &) {}
    virtual void setHighlightLine(int) {}
    void toggleFocus();
    void place(const SDL_Point &pt, std::vector<SDL_Rect> &drawn);
    void move(const SDL_Point &dp);
    virtual void draw(SDL_Renderer *s);
    virtual bool keyCaptured(const SDL_KeyboardEvent &k) const;
    bool clickCaptured(const SDL_MouseButtonEvent &b) const;
    virtual void handleKey(const SDL_KeyboardEvent &k);
    virtual void handleTextInput(const SDL_TextInputEvent &t);
    virtual void handleClick(const SDL_MouseButtonEvent &) {}
};

#endif // TEXTBOX_H
