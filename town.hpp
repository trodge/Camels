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

#include <sqlite3.h>

#include "business.hpp"
#include "draw.hpp"
#include "loadbar.hpp"
#include "nation.hpp"
#include "traveler.hpp"

struct GameData;

class Nation;
class Traveler;

class Town : public Focusable {
    unsigned int id;
    std::unique_ptr<TextBox> box;
    const Nation *nation = nullptr;
    double longitude, latitude;
    bool coastal;
    unsigned long population;
    unsigned int townType;
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
    Town(unsigned int i, const std::vector<std::string> &nms, const Nation *nt, double lng, double lat, bool ctl,
         unsigned long ppl, unsigned int tT, const std::map<std::pair<unsigned int, unsigned int>, double> &fFs, int fS,
         Printer &pr);
    Town(const Save::Town *t, const std::vector<Nation> &ns, int fS, Printer &pr);
    bool operator==(const Town &other) const;
    unsigned int getId() const { return id; }
    std::string getName() const { return box->getText()[0]; }
    const Nation *getNation() const { return nation; }
    double getLongitude() const { return longitude; }
    double getLatitude() const { return latitude; }
    unsigned long getPopulation() const { return population; }
    unsigned int getTownType() const { return townType; }
    const std::vector<Business> &getBusinesses() const { return businesses; }
    const std::vector<Good> &getGoods() const { return goods; }
    const Good &getGood(size_t i) const { return goods[i]; }
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
    void drawRoutes(SDL_Renderer *s);
    void drawDot(SDL_Renderer *s);
    void drawText(SDL_Renderer *s) { box->draw(s); }
    void update(unsigned int e);
    void take(Good &g);
    void put(Good &g);
    void generateTravelers(const GameData &gD, std::vector<std::shared_ptr<Traveler>> &ts);
    double dist(const Town *t) const;
    void loadNeighbors(std::vector<Town> &ts, const std::vector<unsigned int> &nIds);
    void findNeighbors(std::vector<Town> &ts, SDL_Surface *mS, int mox, int moy);
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
