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

#include "textbox.hpp"

TextBox::TextBox(const SDL_Rect &rt, const std::vector<std::string> &tx, const SDL_Color &fg, const SDL_Color &bg,
                 unsigned int i, bool iN, int b, int r, int fS, SDL_Surface *img, Printer &pr)
    : rect(rt), fixedSize(rt.w && rt.h), text(tx), foreground(fg), background(bg), id(i), isNation(iN), border(b), radius(r),
      fontSize(fS), image(img), printer(pr) {
    setText(tx);
}

TextBox::TextBox(const SDL_Rect &rt, const std::vector<std::string> &tx, const SDL_Color &fg, const SDL_Color &bg,
                 unsigned int i, bool iN, int b, int r, int fS, Printer &pr)
    : TextBox(rt, tx, fg, bg, i, iN, b, r, fS, nullptr, pr) {}

TextBox::TextBox(const std::vector<std::string> &tx, const SDL_Color &fg, const SDL_Color &bg, unsigned int i, bool iN,
                 int b, int r, int fS, Printer &pr)
    : TextBox({0, 0, 0, 0}, tx, fg, bg, i, iN, b, r, fS, pr) {}

TextBox::TextBox(const SDL_Rect &rt, const std::vector<std::string> &tx, const SDL_Color &fg, const SDL_Color &bg,
                 unsigned int i, int b, int r, int fS, Printer &pr)
    : TextBox(rt, tx, fg, bg, i, false, b, r, fS, pr) {}

TextBox::TextBox(const SDL_Rect &rt, const std::vector<std::string> &tx, const SDL_Color &fg, const SDL_Color &bg, int b,
                 int r, int fS, Printer &pr)
    : TextBox(rt, tx, fg, bg, 0u, b, r, fS, pr) {}

void TextBox::setText() {
    // Renders the text using the printer. Call any time text changes.
    if (!fixedSize) {
        // Get rid of old width and height and move rectangle to center.
        rect.x += rect.w / 2;
        rect.y += rect.h / 2;
        rect.w = 0;
        rect.h = 0;
    }
    if (isNation)
        printer.setNationId(id);
    else
        printer.setNationId(0);
    if (invColors)
        printer.setColors(background, foreground);
    else
        printer.setColors(foreground, background);
    printer.setSize(fontSize);
    lineHeight = printer.getFontHeight();
    lines = text.size();
    if (rect.h && lines > size_t(rect.h / lineHeight)) {
        // Truncate lines of text if they won'tx fit in box of fixed size.
        lines = static_cast<size_t>(rect.h / lineHeight);
        text = std::vector<std::string>(text.begin(),
                                        text.begin() + static_cast<std::vector<std::string>::difference_type>(lines));
    }
    surface = printer.print(text, rect, border, radius, image);
    updateTexture = true;

    if (!fixedSize) {
        // Rectagle coordinates are at center, move them to top-left corner.
        rect.x -= rect.w / 2;
        rect.y -= rect.h / 2;
    }
}

void TextBox::setBorder(int b) {
    rect.x += (border - b);
    rect.y += (border - b);
    rect.w += (b - border) * 2;
    rect.h += (b - border) * 2;
    border = b;
    setText();
}

void TextBox::setInvColors(bool i) {
    invColors = i;
    setText();
}

void TextBox::setClicked(bool c) {
    clicked = c;
    setInvColors(c);
}

void TextBox::place(int x, int y, std::vector<SDL_Rect> &drawn) {
    rect.x = x - rect.w / 2;
    rect.y = y - rect.h / 2;
    SDL_Rect r = rect;
    bool c = true;
    while (c) {
        c = false;
        for (auto &d : drawn)
            if (SDL_HasIntersection(&d, &r)) {
                c = true;
                break;
            }
        if (c)
            r.y -= r.h / 2;
    }
    rect = r;
    drawn.push_back(r);
}

void TextBox::move(int dx, int dy) {
    rect.x += dx;
    rect.y += dy;
}

void TextBox::draw(SDL_Renderer *s) {
    // Copy this TextBox's texture onto s, updating texture if necessary.
    if (updateTexture) {
        texture = sdl::makeTextureFromSurface(s, surface.get());
        updateTexture = false;
    }
    SDL_RenderCopy(s, texture.get(), nullptr, &rect);
}

bool TextBox::keyCaptured(const SDL_KeyboardEvent &k) const {
    switch (k.keysym.sym) {
    case SDLK_ESCAPE:
    case SDLK_TAB:
        return false;
    default:
        return true;
    }
}

bool TextBox::clickCaptured(const SDL_MouseButtonEvent &b) const {
    SDL_Point mp;
    mp.x = b.x;
    mp.y = b.y;
    return (SDL_PointInRect(&mp, &rect));
}

void TextBox::handleKey(const SDL_KeyboardEvent &k) {
    if (k.state == SDL_PRESSED)
        switch (k.keysym.sym) {
        case SDLK_BACKSPACE:
            if (!text.empty() && !text.back().empty())
                text.back().pop_back();
            setText();
        default:
            return;
        }
}

void TextBox::handleTextInput(const SDL_TextInputEvent &tx) {
    if (!text.empty()) {
        text.back().append(tx.text);
        setText();
    }
}

void Pager::addPage(std::vector<std::unique_ptr<TextBox>> &bxs) {
    // Move parameter boxes and pagers into member vectors giving them a new page. Sets page to that page.
    size_t boxCount = boxes.size();
    indices.push_back(boxCount);
    pageIt = indices.end() - 1;
    boxCount += bxs.size();
    boxes.reserve(boxCount);
    std::move(bxs.begin(), bxs.end(), std::back_inserter(boxes));
    bxs.clear();
}

void Pager::advancePage() {
    // Advance to next page. Prevent advancing past last page.
    if (pageIt != indices.end() - 1)
        ++pageIt;
}

void Pager::recedePage() {
    // Recede to previous page. Prevent receding past first page.
    if (pageIt != indices.begin() + 1)
        --pageIt;
}

void Pager::draw(SDL_Renderer *s) {
    // Draw all boxes on the current page.
    if (!indices.size())
        return;
    auto nextPageIt = pageIt + 1;
    auto stopIt = nextPageIt == indices.end() ? boxes.end() : boxes.begin() + *nextPageIt;
    for (auto bxIt = boxes.begin() + *pageIt; bxIt != stopIt; ++bxIt)
        (*bxIt)->draw(s);
}
