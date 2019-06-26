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

#ifndef MATERIAL_H
#define MATERIAL_H

#include <algorithm>
#include <cmath>
#include <deque>
#include <functional>
#include <iostream>
#include <memory>
#include <numeric>
#include <string>
#include <utility>
#include <vector>

#include "save_generated.h"

#include "constants.h"
#include "menubutton.h"
#include "printer.h"
#include "settings.h"

struct CombatStat {
    int statId, partId, attack, type, speed;
    // attack types are 1: bash, 2: slash, 3: stab
    std::array<int, 3> defense;
};

class Material {
    unsigned int id;
    std::string name;
    double amount;
    double consumption;
    double demandSlope;
    double demandIntercept;
    double minPrice;
    struct PerishCounter {
        int time;
        double amount;
        bool operator<(const PerishCounter &b) const { return time < b.time; }
    };
    std::deque<PerishCounter> perishCounters;
    std::vector<CombatStat> combatStats;
    double lastAmount = 0;

  public:
    Material(unsigned int i, const std::string &n, double a, double c, double dS, double dI);
    Material(unsigned int i, const std::string &n, double c, double dS, double dI);
    Material(unsigned int i, const std::string &n, double a);
    Material(unsigned int i, const std::string &n);
    Material(unsigned int i, double a);
    Material(unsigned int i);
    Material(const Save::Material *m);
    bool operator==(const Material &other) const { return id == other.id; }
    bool operator!=(const Material &other) const { return id != other.id; }
    bool operator<(const Material &other) const { return id < other.id; }
    unsigned int getId() const { return id; }
    std::string getName() const { return name; }
    double getAmount() const { return amount; }
    double getConsumption() const { return consumption; }
    double getDemandSlope() const { return demandSlope; }
    double getMaxPrice() const { return demandIntercept; }
    double getMinPrice() const { return minPrice; }
    double getPrice(double q) const;
    double getPrice() const;
    double getCost(double q) const;
    double getQuantity(double p, double *e) const;
    double getQuantity(double p) const;
    double getQuantum(double c) const;
    const std::vector<CombatStat> &getCombatStats() const { return combatStats; }
    double getBuyScore(double p, double b) const {
        if (p)
            return b / p;
        return 0;
    } // score selling at price p
    double getSellScore(double p, double s) const {
        if (s)
            return p / s;
        return 0;
    } // score buying at price p
    void setAmount(double a) { amount = a; }
    void setCombatStats(const std::vector<CombatStat> &cSs) { combatStats = cSs; }
    void assignConsumption(std::array<double, 3> c);
    void assignConsumption(int p);
    void take(Material &m);
    void use(double a);
    void put(Material &m);
    void create(double a);
    double perish(int e, double p);
    double consume(int e);
    std::unique_ptr<MenuButton> button(bool aS, int gI, const std::string &gN, bool gS, SDL_Rect r, SDL_Color fgr,
                                       SDL_Color bgr, int fS, std::function<void()> f) const;
    void updateButton(bool gS, double oV, int rC, TextBox *b) const;
    void adjustDemand(double d);
    void fixDemand(double m);
    void saveDemand(int p, std::string &u) const;
    flatbuffers::Offset<Save::Material> save(flatbuffers::FlatBufferBuilder &b) const;
};

void dropTrail(std::string *t, int dK);

#endif // MATERIAL_H
