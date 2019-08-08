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

#include <algorithm>
#include <memory>
#include <vector>

#include "textbox.hpp"

#ifndef PAGER_H
#define PAGER_H

class Pager {
    std::vector<std::unique_ptr<TextBox>> boxes; // boxes this pager manages
    std::vector<size_t> indices;                 // indices to page breaks in boxes
    std::vector<size_t>::iterator pageIt;        // index of beginning indices of current page
    std::array<std::vector<std::unique_ptr<TextBox>>::iterator, 2> visible;
    SDL_Rect bounds{0, 0, 0, 0};
    void setVisible();

public:
    size_t boxCount() { return boxes.size(); }
    TextBox *getVisible(size_t idx);
    std::vector<TextBox *> getVisible();
    std::vector<TextBox *> getAll();
    size_t pageCount() const { return indices.size(); }
    size_t visibleCount() const { return visible[1] - visible[0]; }
    const SDL_Rect &getBounds() { return bounds; }
    void setBounds(const SDL_Rect &bnds) { bounds = bnds; }
    void setBounds();
    void addBox(std::unique_ptr<TextBox> &&bx);
    void addPage(std::vector<std::unique_ptr<TextBox>> &bxs);
    void reset();
    void recedePage();
    void advancePage();
    void setPage(size_t pg);
    int getKeyedIndex(const SDL_KeyboardEvent &k);
    int getClickedIndex(const SDL_MouseButtonEvent &b);
    void draw(SDL_Renderer *s);
};

#endif
