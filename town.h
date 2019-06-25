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
#include <string>
#include <utility>

#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>

#include "save_generated.h"

#include "business.h"
#include "draw.h"
#include "loadbar.h"
#include "nation.h"
#include "traveler.h"

struct GameData;

class Traveler;

class Town : public Focusable {
    int id;
    std::unique_ptr<TextBox> box;
    const Nation *nation = nullptr;
    double longitude, latitude;
    bool coastal;
    int population;
    int townType;
    std::vector<Business> businesses;
    std::vector<Good> goods;
    std::vector<Town *> neighbors;
    int businessCounter;
    bool maxGoods = false; // town creates maximum goods for testing purposes
    std::vector<std::shared_ptr<Traveler>> travelers;
    std::uniform_int_distribution<int> travelersDis;
    int dpx, dpy;
    int distSq(int x, int y) const;
    double dist(int x, int y) const;
    void setMax();

  public:
    Town(sqlite3_stmt *q, const std::vector<Nation> &ns, const std::vector<Business> &bs,
         const std::map<std::pair<int, int>, double> &fFs);
    Town(const Save::Town *t, const std::vector<Nation> &ns);
    bool operator==(const Town &other) const;
    int getId() const { return id; }
    std::string getName() const { return box->getText()[0]; }
    const Nation *getNation() const { return nation; }
    double getLongitude() const { return longitude; }
    double getLatitude() const { return latitude; }
    int getPopulation() const { return population; }
    int getTownType() const { return townType; }
    const std::vector<Good> &getGoods() const { return goods; }
    const Good &getGood(size_t i) { return goods[i]; }
    const std::vector<Town *> &getNeighbors() const { return neighbors; }
    const std::vector<std::shared_ptr<Traveler>> &getTravelers() const { return travelers; }
    void addTraveler(std::shared_ptr<Traveler> t) { travelers.push_back(t); }
    void removeTraveler(const std::shared_ptr<Traveler> t);
    void clearTravelers() { travelers.clear(); }
    void focus() { box->changeBorder(1); }
    void unFocus() { box->changeBorder(-1); }
    bool clickCaptured(const SDL_MouseButtonEvent &b) const { return box->clickCaptured(b); }
    void toggleMaxGoods() { maxGoods = not maxGoods; }
    void placeDot(std::vector<SDL_Rect> &drawn, int ox, int oy, double s);
    void placeText(std::vector<SDL_Rect> &drawn) { box->place(dpx, dpy, drawn); }
    void drawRoutes(SDL_Surface *s);
    void drawDot(SDL_Surface *s);
    void drawText(SDL_Surface *s) { box->draw(s); }
    void update(int e);
    void take(Good &g);
    void put(Good &g);
    void generateTravelers(const GameData *gD, std::vector<std::shared_ptr<Traveler>> &ts);
    double dist(const Town *t) const;
    void loadNeighbors(std::vector<Town> &ts, const std::vector<size_t> &nIs);
    void findNeighbors(std::vector<Town> &ts, const SDL_Surface *map, int mox, int moy);
    void connectRoutes();
    void saveNeighbors(std::string &i) const;
    void adjustAreas(const std::vector<MenuButton *> &rBs, double d);
    void resetGoods();
    void saveFrequencies(std::string &u) const;
    void adjustDemand(const std::vector<MenuButton *> &rBs, double d);
    void saveDemand(std::string &u) const;
    flatbuffers::Offset<Save::Town> save(flatbuffers::FlatBufferBuilder &b) const;
};

#endif // TOWN_H
