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

class TextBox {
protected:
    SDL_Rect rect;
    bool fixedSize, canFocus = false;
    std::vector<std::string> text;
    size_t lines;
    SDL_Color foreground;
    SDL_Color background;
    unsigned int id;
    bool isNation;
    int border;
    int radius; // radius of rounded corner circles
    bool invColors = false;
    bool clicked = false;
    int fontSize;
    int lineHeight = -1;
    sdl::SurfacePtr surface;
    sdl::TexturePtr texture;
    bool updateTexture = false; // whether the texture needs updating to match the surface
    SDL_Surface *image;
    Printer &printer;
    TextBox(Printer &pr) : printer(pr) {}
    void setBorder(int b);

public:
    TextBox(const SDL_Rect &rt, const std::vector<std::string> &tx, const SDL_Color &fg, const SDL_Color &bg,
            unsigned int i, bool iN, int b, int r, int fS, SDL_Surface *img, Printer &pr);
    TextBox(const SDL_Rect &rt, const std::vector<std::string> &tx, const SDL_Color &fg, const SDL_Color &bg,
            unsigned int i, bool iN, int b, int r, int fS, Printer &pr);
    TextBox(const std::vector<std::string> &tx, const SDL_Color &fg, const SDL_Color &bg, unsigned int i, bool iN,
            int b, int r, int fS, Printer &pr);
    TextBox(const SDL_Rect &rt, const std::vector<std::string> &tx, const SDL_Color &fg, const SDL_Color &bg,
            unsigned int i, int b, int r, int fS, Printer &pr);
    TextBox(const SDL_Rect &rt, const std::vector<std::string> &tx, const SDL_Color &fg, const SDL_Color &bg, int b,
            int r, int fS, Printer &pr);
    virtual ~TextBox() {}
    virtual SDL_Keycode getKey() const { return SDLK_UNKNOWN; }
    const SDL_Rect &getRect() const { return rect; }
    bool getCanFocus() const { return canFocus; }
    const std::vector<std::string> &getText() const { return text; }
    const std::string &getText(size_t i) { return text[i]; }
    virtual const std::string &getItem() const { return text.back(); }
    bool getClicked() const { return clicked; }
    unsigned int getId() const { return id; }
    int getDivisor() const { return fontSize; }
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
    void toggleLock() { canFocus = !canFocus; }
    virtual void changeBorder(int dBS) { setBorder(border + dBS); }
    void setInvColors(bool i);
    void setClicked(bool c);
    virtual void setKey(const SDL_Keycode &) {}
    virtual void setHighlightLine(int) {}
    void place(int x, int y, std::vector<SDL_Rect> &drawn);
    void move(int dx, int dy);
    virtual void draw(SDL_Renderer *s);
    virtual bool keyCaptured(const SDL_KeyboardEvent &k) const;
    bool clickCaptured(const SDL_MouseButtonEvent &b) const;
    virtual void handleKey(const SDL_KeyboardEvent &k);
    virtual void handleTextInput(const SDL_TextInputEvent &t);
    virtual void handleClick(const SDL_MouseButtonEvent &) {}
};

class Pager {
    std::vector<std::unique_ptr<TextBox>> &boxes; // boxes this pager works on
    std::vector<size_t> indices;                  // indices of page breaks in boxes
    std::vector<size_t>::iterator page;           // iterator to index of current page
    std::vector<std::unique_ptr<TextBox>> hidden; // boxes not shown on current page
public:
    Pager(std::vector<std::unique_ptr<TextBox>> &bxs);
    void addIndex(size_t idx) { indices.push_back(idx); }
    void hideBoxes();
    void nextPage();
};

#endif // TEXTBOX_H
