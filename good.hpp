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

#ifndef GOOD_H
#define GOOD_H

#include <algorithm>
#include <numeric>
#include <unordered_map>
#include <vector>

#include "save_generated.h"

#include "constants.hpp"
#include "menubutton.hpp"
#include "printer.hpp"
#include "settings.hpp"

struct CombatStat {
    unsigned int statId, partId, attack, type, speed;
    // attack types are 1: bash, 2: slash, 3: stab
    std::array<unsigned int, 3> defense;
};

class Good {
    unsigned int goodId, materialId = 0, fullId = 0;
    std::string goodName, materialName, fullName;
    double amount = 0, perish = 0, carry = 0;
    std::string measure;
    bool split;
    double maximum = 0;
    double consumptionRate;
    double demandSlope;
    double demandIntercept;
    double minPrice;
    unsigned int ammoId = 0; // good id of good this good shoots
    std::vector<CombatStat> combatStats;
    double lastAmount = 0;
    SDL_Surface *image = nullptr;
    struct PerishCounter {
        int time;
        double amount;
        bool operator<(const PerishCounter &b) const { return time < b.time; }
    };
    std::deque<PerishCounter> perishCounters;
    std::string completeName() { return goodName == materialName ? goodName : materialName + " " + goodName; }
    void updateButton(std::string &aT, TextBox *btn) const;
    void enforceMaximum();

public:
    Good(unsigned int gId, const std::string &gNm, const std::string &msr, unsigned int aId)
        : goodId(gId), goodName(gNm), measure(msr), ammoId(aId) {} // constructor for loading from goods table
    Good(const Good &gd, const Good &mt, unsigned int fId, double pr, double cr)
        : goodId(gd.goodId), materialId(mt.goodId), fullId(fId), goodName(gd.goodName), materialName(mt.goodName),
          fullName(completeName()), perish(pr), carry(cr), measure(gd.measure), split(!measure.empty()) {
    } // constructor for loading from materials table
    Good(const Good &gd, double amt)
        : goodId(gd.goodId), goodName(gd.goodName), amount(amt), measure(gd.measure), split(!measure.empty()) {} // constructor for business inputs and outputs
    Good(unsigned int fId, const std::string &fNm, double amt, const std::vector<CombatStat> &cSs, SDL_Surface *img)
        : fullId(fId), fullName(fNm), amount(amt), combatStats(cSs), image(img) {} // constructor for equipment
    Good(unsigned int fId, double amt) : fullId(fId), amount(amt) {}               // constructor for transfer goods
    Good(unsigned int fId, const std::string &fNm, double amt, const std::string &msr)
        : fullId(fId), fullName(fNm), amount(amt), measure(msr), split(!measure.empty()) {
    } // constructor for trade goods
    Good(const Save::Good *svG)
        : goodId(svG->goodId()), materialId(svG->materialId()), fullId(svG->fullId()), goodName(svG->goodName()->str()),
          materialName(svG->materialName()->str()), fullName(completeName()), amount(svG->amount()),
          perish(svG->perish()), carry(svG->carry()), measure(svG->measure()->str()), split(!measure.empty()),
          consumptionRate(svG->consumptionRate()), demandSlope(svG->demandSlope()), demandIntercept(svG->demandIntercept()),
          minPrice(demandIntercept / Settings::getMinPriceDivisor()), lastAmount(amount) {}
    flatbuffers::Offset<Save::Good> save(flatbuffers::FlatBufferBuilder &b) const;
    bool operator==(const Good &other) const { return goodId == other.goodId && materialId == other.materialId; }
    bool operator!=(const Good &other) const { return goodId != other.goodId || materialId != other.materialId; }
    bool operator<(const Good &other) const { return fullId < other.fullId; }
    unsigned int getGoodId() const { return goodId; }
    unsigned int getMaterialId() const { return materialId; }
    unsigned int getFullId() const { return fullId; }
    const std::string &getGoodName() const { return goodName; }
    const std::string &getMaterialName() const { return materialName; }
    const std::string &getFullName() const { return fullName; }
    std::string businessText() const;
    double getAmount() const { return amount; }
    double getPerish() const { return perish; }
    double getCarry() const { return carry; }
    double weight() const { return amount * carry; }
    const std::string &getMeasure() const { return measure; }
    bool getSplit() const { return split; }
    double getMaximum() const { return maximum; }
    unsigned int getAmmoId() const { return ammoId; }
    double getConsumptionRate() const { return consumptionRate; }
    double getDemandSlope() const { return demandSlope; }
    double getMaxPrice() const { return demandIntercept; }
    double getMinPrice() const { return minPrice; }
    double price(double qtt) const;
    double price() const;
    double cost(double qtt) const;
    double value() const { return price() * amount; }
    double quantity(double prc, double &elTm) const;
    double quantity(double prc) const;
    double quantum(double c) const;
    const std::vector<CombatStat> &getCombatStats() const { return combatStats; }
    SDL_Surface *getImage() const { return image; }
    void setAmount(double a) { amount = a; }
    void setConsumption(const std::array<double, 3> &cnsptn);
    void setCombatStats(const std::vector<CombatStat> &cSs) { combatStats = cSs; }
    void setImage(SDL_Surface *img) { image = img; }
    void scale(double ppl);
    void take(Good &gd);
    void put(Good &gd);
    void use(double amt);
    void use() { use(amount); }
    void create(double amt);
    void create() { create(maximum); }
    void update(unsigned int elTm, double yrLn);
    void updateButton(TextBox *btn) const;
    void updateButton(double oV, unsigned int rC, TextBox *btn) const;
    void adjustDemand(double d);
    void fixDemand(double m);
    void saveDemand(unsigned long ppl, std::string &u) const;
    std::string logEntry() const;
    void setMaximum() { maximum = std::abs(consumptionRate) * Settings::getConsumptionSpaceFactor(); }
    void setMaximum(double max) { maximum = std::max(maximum, max); }
    void assignConsumption(const std::unordered_map<unsigned int, std::array<double, 3>> &cs);
    std::unique_ptr<MenuButton> button(bool aS, BoxInfo bI, Printer &pr) const;
    void adjustDemandSlope(double dDS);
};

void dropTrail(std::string &tx, unsigned int dK);

#endif // GOOD_H
