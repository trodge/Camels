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

struct BoxRange {
    std::vector<std::unique_ptr<TextBox>>::iterator start;
    std::vector<std::unique_ptr<TextBox>>::iterator stop;
};

class Pager {
    std::vector<std::unique_ptr<TextBox>> boxes; // boxes this pager manages
    std::vector<size_t> indices;                 // indices to page breaks in boxes
    std::vector<size_t>::iterator pageIt;        // index of beginning indices of current page
    BoxRange visible;
    void setVisible();

  public:
    TextBox *getVisible(size_t idx);
    std::vector<TextBox *> getVisible();
    std::vector<TextBox *> getAll();
    int visibleCount() const;
    void addBox(std::unique_ptr<TextBox> &&bx) { boxes.push_back(std::move(bx)); }
    void addPage(std::vector<std::unique_ptr<TextBox>> &bxs);
    void advancePage();
    void recedePage();
    void toggleLock(size_t idx) { boxes[idx]->toggleLock(); }
    int getClickedIndex(const SDL_MouseButtonEvent &b);
    void draw(SDL_Renderer *s);
};

#endif
