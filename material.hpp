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
#include <deque>
#include <iostream>
#include <memory>
#include <numeric>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

#include "save_generated.h"

#include "constants.hpp"
#include "menubutton.hpp"
#include "printer.hpp"
#include "settings.hpp"

struct CombatStat {
    unsigned int statId, partId, attack, type, speed;
    // stat this material uses in combat
    // part this material can be equpped on
    // power of this material's attack
    // attack type - 1: bash, 2: slash, 3: stab
    // speed when attacking
    std::array<unsigned int, 3> defense;
};

class Material {
    unsigned int id;
    std::string name;
    double amount;
    double perish;
    double carry;
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
    SDL_Surface *image = nullptr;
    void updateButton(std::string &aT, bool gS, TextBox *b) const;

public:
    Material(unsigned int i, const std::string &n, double a, double p, double c, double cs, double dS, double dI, SDL_Surface *img)
        : id(i), name(n), amount(a), perish(p), carry(c), consumption(cs), demandSlope(dS), demandIntercept(dI),
          minPrice(dI / Settings::getMinPriceDivisor()), lastAmount(a), image(img) {}
    Material(unsigned int i, const std::string &n, double cs, double dS, double dI, SDL_Surface *img)
        : Material(i, n, 0, 0, 0, cs, dS, dI, img) {}
    Material(unsigned int i, const std::string &n, double cs, double dS, double dI)
        : Material(i, n, cs, dS, dI, nullptr) {}
    Material(unsigned int i, const std::string &n, double a, SDL_Surface *img)
        : Material(i, n, a, 0, 0, 0, 0, 0, img) {}
    Material(unsigned int i, const std::string &n, double a) : Material(i, n, a, nullptr) {}
    Material(unsigned int i, const std::string &n, double p, double c, SDL_Surface *img)
        : Material(i, n, 0, p, c, 0, 0, 0, img) {}
    Material(unsigned int i, const std::string &n, double p, double c) : Material(i, n, p, c, nullptr) {}
    Material(unsigned int i, const std::string &n) : Material(i, n, 0) {}
    Material(unsigned int i, double a) : Material(i, "", a) {}
    Material(unsigned int i) : Material(i, 0) {}
    Material(const Save::Material *m);
    flatbuffers::Offset<Save::Material> save(flatbuffers::FlatBufferBuilder &b) const;
    bool operator==(const Material &other) const { return id == other.id; }
    bool operator!=(const Material &other) const { return id != other.id; }
    bool operator<(const Material &other) const { return id < other.id; }
    unsigned int getId() const { return id; }
    std::string getName() const { return name; }
    double getAmount() const { return amount; }
    double getPerish() const { return perish; }
    double getCarry() const { return carry; }
    double getConsumption() const { return consumption; }
    double getDemandSlope() const { return demandSlope; }
    double getMaxPrice() const { return demandIntercept; }
    double getMinPrice() const { return minPrice; }
    const std::vector<CombatStat> &getCombatStats() const { return combatStats; }
    double weight() const { return amount * carry; }
    double price(double q) const;
    double price() const;
    double cost(double q) const;
    double quantity(double p, double &e) const;
    double quantity(double p) const;
    double quantum(double c) const;
    double buyScore(double p, double b) const {
        if (p != 0) return b / p;
        return 0;
    } // score selling at price p
    double sellScore(double p, double s) const {
        if (s != 0) return p / s;
        return 0;
    } // score buying at price p
    SDL_Surface *getImage() const { return image; }
    void setAmount(double a) { amount = a; }
    void setCombatStats(const std::vector<CombatStat> &cSs) { combatStats = cSs; }
    void setImage(SDL_Surface *img) { image = img; }
    void setConsumption(const std::array<double, 3> &c);
    void scaleConsumption(unsigned long p);
    void take(Material &m);
    void use(double a);
    void put(Material &m);
    void create(double a);
    double consume(unsigned int e);
    void updateButton(bool gS, TextBox *b) const;
    void updateButton(bool gS, double oV, unsigned int rC, TextBox *b) const;
    void adjustDemand(double d);
    void fixDemand(double m);
    void saveDemand(unsigned long p, std::string &u) const;
};

void dropTrail(std::string &tx, unsigned int dK);

#endif // MATERIAL_H
