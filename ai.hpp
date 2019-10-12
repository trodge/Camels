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

#ifndef AI_H
#define AI_H

#include <functional>
#include <memory>

#include <boost/multi_index/hashed_index.hpp>
#include <boost/multi_index/mem_fun.hpp>
#include <boost/multi_index/member.hpp>
#include <boost/multi_index_container.hpp>

namespace mi = boost::multi_index;

#include "save_generated.h"

#include "property.hpp"
#include "traveler.hpp"

enum class FightChoice;

class Good;

class Town;

class Traveler;

class GoodInfo {
    unsigned int fullId;
    bool owned;
    double limitFactor,      // factor controlling value based on min/max price
        min, max,            // minimum and maximum price of material seen
        estimate, buy, sell; // estimated value, maximum buy price, and minimum sell price
public:
    GoodInfo(unsigned int fId, bool ond);
    GoodInfo(const Save::GoodInfo *ldGdInf);
    Save::GoodInfo save() const;
    unsigned int getFullId() const { return fullId; }
    bool getOwned() const { return owned; }
    double getEstimate() const { return estimate; }
    double getBuy() const { return buy; }
    double getSell() const { return sell; }
    double buyScore(double prc) const { return prc == 0 ? 0 : buy / prc; }    // score selling at price p
    double sellScore(double prc) const { return sell == 0 ? 0 : prc / sell; } // score buying at price p
    void setOwned(bool ond) { owned = ond; }
    void setMinMax(double m) {
        min = std::min(m, min);
        max = std::max(m, max);
    }
    void appraise(double tnPft);
};

struct TownInfo {
    Town *town;                 // pointer to town in question
    double buyScore, sellScore; // highest buy and sell scores for town
};

/**
 * AI which runs non-player travelers
 */
class AI {
    Traveler &traveler;                                   // the traveler this AI controls
    int decisionCounter,                                  // counter for updating AI
        businessCounter;                                  // counter for making business decisions
    EnumArray<double, DecisionCriteria> decisionCriteria; /* buy/sell score
    weight, weapon/armor equip score, tendency to fight/run/yield, looting greed */
    using FlId = mi::const_mem_fun<GoodInfo, unsigned int, &GoodInfo::getFullId>;
    using Ownd = mi::const_mem_fun<GoodInfo, bool, &GoodInfo::getOwned>;
    using FlIdHsh = mi::hashed_unique<mi::tag<struct FullId>, FlId>;
    using OwndHsh = mi::hashed_non_unique<mi::tag<struct Owned>, Ownd>;
    using GoodInfoContainer = boost::multi_index_container<GoodInfo, mi::indexed_by<FlIdHsh, OwndHsh>>;
    GoodInfoContainer goodsInfo;  // known information about each good by full id
    std::vector<TownInfo> nearby; // known information about nearby towns
    AIRole role;                  // behavior for this ai
    void setNearby(const Town *t, const Town *tT, unsigned int i);
    void setLimits();
    double attackScore(const Good &eq, const EnumArray<unsigned int, Stat> &sts) const;
    double attackScore(const std::vector<Good> &eqpmt, const EnumArray<unsigned int, Stat> &sts) const;
    double defenseScore(const Good &eq, const EnumArray<unsigned int, Stat> &sts) const;
    double defenseScore(const std::vector<Good> &eqpmt, const EnumArray<unsigned int, Stat> &sts) const;
    double equipScore(const Good &eq, const EnumArray<unsigned int, Stat> &sts) const;
    double equipScore(const std::vector<Good> &eqpmt, const EnumArray<unsigned int, Stat> &sts) const;
    double equipScore(const Good &eq, const std::vector<Good> &eqpmt, const EnumArray<unsigned int, Stat> &sts) const;
    double lootScore(const Property &ppt);
    void choosePlan(std::vector<BusinessPlan> &plns, BusinessPlan *&bstPln, double dcCt, double &hst);
    void trade();
    void store(const Property *sPpt, const Property &tPpt);
    void equip();
    void attack();
    void pickTown(const Town *tn);

public:
    AI(Traveler &tvl, const EnumArray<double, DecisionCriteria> &dcC, const GoodInfoContainer &gsI, AIRole rl);
    AI(Traveler &tvl) : AI(tvl, Settings::aIDecisionCriteria(), {}, Settings::aIRole()) {}
    AI(Traveler &tvl, const AI &p) : AI(tvl, p.decisionCriteria, p.goodsInfo, p.role) {}
    AI(Traveler &tvl, const Save::AI *ldAI);
    flatbuffers::Offset<Save::AI> save(flatbuffers::FlatBufferBuilder &b) const;
    AIRole getRole() const { return role; }
    FightChoice choice();
    Traveler *target(const std::unordered_set<Traveler *> &enms) const;
    Traveler *lootTarget(const std::unordered_set<Traveler *> &enms);
    void loot();
    void update(unsigned int elTm);
};

#endif // AI_H
