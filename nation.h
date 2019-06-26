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

#ifndef NATION_H
#define NATION_H

#include <iostream>
#include <map>
#include <vector>

#include <SDL2/SDL.h>
#include <sqlite3.h>

#include "good.h"

class Nation {
    unsigned int id;
    std::vector<std::string> names;
    std::string adjective;
    SDL_Color foreground, background, highlight, dot;
    std::string religion;
    std::vector<std::string> travelerNames;
    std::vector<Good> goods;
    std::map<std::pair<int, int>, double> frequencies;

  public:
    Nation(sqlite3_stmt *q, const std::vector<Good> &gs);
    bool operator==(const Nation &other) const;
    unsigned int getId() const { return id; }
    const std::vector<std::string> getNames() const { return names; }
    const std::string getAdjective() const { return adjective; }
    const SDL_Color &getForeground() const { return foreground; }
    const SDL_Color &getBackground() const { return background; }
    const SDL_Color &getHighlight() const { return highlight; }
    const SDL_Color &getDotColor() const { return dot; }
    double getFrequency(int b, int m) const;
    const std::vector<Good> &getGoods() const { return goods; }
    std::string randomName() const;
    void loadData(sqlite3 *c);
};

#endif // NATION_H
