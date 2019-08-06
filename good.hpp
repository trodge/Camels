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
    unsigned int goodId, materialId, fullId;
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
    void setFullName();
    void updateButton(std::string &aT, bool gS, TextBox *btn) const;
    void enforceMaximum();

public:
    Good(unsigned int gId, unsigned int mId, unsigned int fId, const std::string &gNm, const std::string &mNm,
         double amt, double prR, double crW, const std::string &msW, double csR, double dmS, double dmI, SDL_Surface *img)
        : goodId(gId), materialId(mId), fullId(fId), goodName(gNm), materialName(mNm), amount(amt), perish(prR),
          carry(crW), measure(msW), split(!msW.empty()), consumptionRate(csR), demandSlope(dmS), demandIntercept(dmI),
          minPrice(dmI / Settings::getMinPriceDivisor()), lastAmount(amt), image(img) {
        setFullName();
    }
    Good(unsigned int gId, const std::string &gNm, const std::string &msr, unsigned int aId)
        : goodId(gId), goodName(gNm), measure(msr), ammoId(aId) {} // constructor for loading from goods table
    Good(const Good &dbGd, const Good &dbMt, unsigned int fId, double pr, double cr)
        : goodId(dbGd.goodId), materialId(dbMt.goodId), fullId(fId), goodName(dbGd.goodName),
          materialName(dbMt.goodName), perish(pr), carry(cr), measure(dbGd.measure), split(!measure.empty()) {
        setFullName();
    }                                                               // constructor for loading from materials table
    Good(unsigned int fId, double amt) : fullId(fId), amount(amt) {} // constructor for transfer goods
    Good(unsigned int gId, const std::string &gNm, double amt, const std::string &msr)
        : goodId(gId), goodName(gNm), amount(amt), measure(msr) {} // constructor for business goods
    Good(const Save::Good *svG)
        : goodId(svG->goodId()), materialId(svG->materialId()), fullId(svG->fullId()), goodName(svG->goodName()->str()),
          materialName(svG->materialName()->str()), amount(svG->amount()), perish(svG->perish()), carry(svG->carry()),
          measure(svG->measure()->str()), split(!measure.empty()), consumptionRate(svG->consumptionRate()),
          demandSlope(svG->demandSlope()), demandIntercept(svG->demandIntercept()),
          minPrice(demandIntercept / Settings::getMinPriceDivisor()), lastAmount(amount) {
        setFullName();
    }
    flatbuffers::Offset<Save::Good> save(flatbuffers::FlatBufferBuilder &b) const;
    bool operator==(const Good &other) const { return goodId == other.goodId and materialId == other.materialId; }
    bool operator!=(const Good &other) const { return goodId != other.goodId or materialId != other.materialId; }
    bool operator<(const Good &other) const { return fullId < other.fullId; }
    unsigned int getGoodId() const { return goodId; }
    unsigned int getMaterialId() const { return materialId; }
    unsigned int getFullId() const { return fullId; }
    const std::string &getGoodName() const { return goodName; }
    const std::string &getMaterialName() const { return materialName; }
    const std::string &getFullName() const { return fullName; }
    std::string businessText() const;
    double getPerish() const { return perish; }
    double getCarry() const { return carry; }
    double getAmount() const { return amount; }
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
    double quantity(double prc, double &elTm) const;
    double quantity(double prc) const;
    double quantum(double c) const;
    const std::vector<CombatStat> &getCombatStats() const { return combatStats; }
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
    void setConsumption(const std::array<double, 3> &cnsptn);
    void scaleConsumption(unsigned long ppl);
    void take(Good &gd);
    void put(Good &gd);
    void use(double amt);
    void use() { use(amount); }
    void create(double amt);
    void create() { create(maximum); }
    void update(unsigned int elTm);
    void updateButton(bool gS, TextBox *btn) const;
    void updateButton(bool gS, double oV, unsigned int rC, TextBox *btn) const;
    void adjustDemand(double d);
    void fixDemand(double m);
    void saveDemand(unsigned long ppl, std::string &u) const;
    std::string logEntry() const;
    void setCombatStats(const std::unordered_map<unsigned int, std::vector<CombatStat>> &cSs);
    void setMaximum() { maximum = std::abs(consumptionRate) * Settings::getConsumptionSpaceFactor(); }
    void setMaximum(double max) { maximum = std::max(maximum, max); }
    void assignConsumption(const std::unordered_map<unsigned int, std::array<double, 3>> &cs);
    std::unique_ptr<MenuButton> button(bool aS, BoxInfo bI, Printer &pr) const;
    void adjustDemandSlope(double dDS);
};

void dropTrail(std::string &tx, unsigned int dK);

#endif // GOOD_H
