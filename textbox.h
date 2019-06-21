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

#include "focusable.h"

class TextBox : public Focusable {
  protected:
    SDL_Rect rect;
    bool fixedSize;
    std::vector<std::string> text;
    size_t lines;
    SDL_Color foreground;
    SDL_Color background;
    int id;
    bool isNation;
    int border;
    bool invColors = false;
    bool clicked = false;
    int fontSize;
    int lineHeight = -1;
    SDL_Surface *ts = nullptr;
    TextBox() {}
    void setBorder(int b);

  public:
    TextBox(SDL_Rect r, const std::vector<std::string> &t, SDL_Color fg, SDL_Color bg, int i, bool iN, int b, int fS);
    TextBox(SDL_Rect r, const std::vector<std::string> &t, SDL_Color fg, SDL_Color bg, int i, int b, int fS);
    TextBox(SDL_Rect r, const std::vector<std::string> &t, SDL_Color fg, SDL_Color bg, int b, int fS);
    virtual ~TextBox();
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
    void toggleLock() { canFocus = not canFocus; }
    void changeBorder(int db) { setBorder(border + db); }
    void setInvColors(bool i);
    void setClicked(bool c);
    virtual void setKey(const SDL_Keycode &) {}
    virtual void setHighlightLine(int) {}
    virtual SDL_Keycode getKey() const { return SDLK_UNKNOWN; }
    const SDL_Rect &getRect() const { return rect; }
    void place(int x, int y, std::vector<SDL_Rect> &drawn);
    virtual void draw(SDL_Surface *s) {
        SDL_Rect r = rect;
        SDL_BlitSurface(ts, NULL, s, &r);
    }
    const std::vector<std::string> &getText() const { return text; }
    const std::string &getText(size_t i) { return text[i]; }
    virtual const std::string &getItem() const { return text.back(); }
    bool getClicked() const { return clicked; }
    int getId() const { return id; }
    int getFontSize() const { return fontSize; }
    virtual bool keyCaptured(const SDL_KeyboardEvent &k) const;
    bool clickCaptured(const SDL_MouseButtonEvent &b) const;
    virtual void handleKey(const SDL_KeyboardEvent &k);
    virtual void handleTextInput(const SDL_TextInputEvent &t);
    virtual void handleClick(const SDL_MouseButtonEvent &) {}
    virtual void focus() {
        SDL_StartTextInput();
        changeBorder(2);
    }
    virtual void unFocus() {
        SDL_StopTextInput();
        changeBorder(-2);
    }
    virtual int getHighlightLine() const { return -1; }
};

#endif // TEXTBOX_H
