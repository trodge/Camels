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

#ifndef TOWN_H
#define TOWN_H

#include <map>
#include <memory>
#include <string>
#include <utility>

#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>

#include <sqlite3.h>

#include "business.hpp"
#include "draw.hpp"
#include "loadbar.hpp"
#include "nation.hpp"
#include "property.hpp"
#include "textbox.hpp"
#include "traveler.hpp"

struct GameData;
class Nation;
struct Contract;
class Traveler;

class Town {
    unsigned int id;
    const Nation *nation = nullptr;
    std::unique_ptr<TextBox> box;
    Position position;
    Property property;
    std::vector<Town *> neighbors;
    std::vector<Traveler *> travelers;
    std::vector<Contract> bids;

public:
    Town(unsigned int i, const std::vector<std::string> &nms, const Nation *nt, double lng, double lat,
         TownType tT, bool ctl, long unsigned int ppl, Printer &pr);
    Town(const Save::Town *ldTn, const std::vector<Nation> &ns, Printer &pr);
    flatbuffers::Offset<Save::Town> save(flatbuffers::FlatBufferBuilder &b) const;
    bool operator==(const Town &other) const;
    unsigned int getId() const { return id; }
    TextBox *getBox() const { return box.get(); }
    std::string getName() const { return box->getText()[0]; }
    const Nation *getNation() const { return nation; }
    const Position &getPosition() const { return position; }
    const Property &getProperty() const { return property; }
    const std::vector<Town *> &getNeighbors() const { return neighbors; }
    const std::vector<Traveler *> &getTravelers() const { return travelers; }
    const std::vector<Contract> &getBids() const { return bids; }
    void clearTravelers() { travelers.clear(); }
    void removeTraveler(const Traveler *t);
    void addTraveler(Traveler *t) { travelers.push_back(t); }
    Contract takeBid(size_t idx);
    void addBid(const Contract &bd) { bids.push_back(bd); }
    bool clickCaptured(const SDL_MouseButtonEvent &b) const { return box->clickCaptured(b); }
    void toggleMaxGoods() { property.toggleMaxGoods(); }
    void placeDot(std::vector<SDL_Rect> &drawn, const SDL_Point &ofs, double s);
    void placeText(std::vector<SDL_Rect> &drawn) { box->place(position.getPoint(), drawn); }
    void draw(SDL_Renderer *s);
    void reset() { property.reset(); }
    void update(unsigned int e);
    void take(Good &g);
    void put(Good &g);
    void generateTravelers(const GameData &gD, std::vector<std::unique_ptr<Traveler>> &tvlrs);
    int distSq(const Town *t) const;
    void addNeighbor(Town *t) { neighbors.push_back(t); }
    void findNeighbors(std::vector<Town> &ts, SDL_Surface *mS, const SDL_Point &mOs);
    void connectRoutes();
    void adjustAreas(const std::vector<MenuButton *> &rBs, double d) { property.adjustAreas(rBs, d); }
    void saveFrequencies(std::string &u) const;
    void adjustDemand(const std::vector<MenuButton *> &rBs, double d) { property.adjustDemand(rBs, d); }
    void saveDemand(std::string &u) const;
};

class Route {
    std::array<Town *, 2> towns;

public:
    Route(Town *fT, Town *tT);
    Route(const Save::Route *rt, std::vector<Town> &ts);
    const std::array<Town *, 2> &getTowns() { return towns; }
    void draw(SDL_Renderer *s);
    void saveData(std::string &i) const;
    flatbuffers::Offset<Save::Route> save(flatbuffers::FlatBufferBuilder &b) const;
};

#endif // TOWN_H
