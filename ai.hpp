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

struct GoodInfo {
    unsigned int fullId;
    bool owned;
    double limitFactor = 0; // factor controlling value based on min/max price
    double minPrice = 0,
           maxPrice = 0; // minimum and maximum price of material nearby
    double estimate = 0, buy = 0,
           sell = 0; // estimated value, maximum buy price, and minimum sell price
};

struct TownInfo {
    Town *town;                 // pointer to town in question
    double buyScore, sellScore; // highest buy and sell scores for town
};

/**
 * AI which updates non-player travelers
 */
class AI {
    Traveler &traveler;                                   // the traveler this AI controls
    int decisionCounter;                                  // counter for updatening AI
    EnumArray<double, DecisionCriteria> decisionCriteria; /* buy/sell score
    weight, weapon/armor equip score, tendency to fight/run/yield, looting greed */
    using FlId = mi::member<GoodInfo, unsigned int, &GoodInfo::fullId>;
    using Ownd = mi::member<GoodInfo, bool, &GoodInfo::owned>;
    using FlIdHsh = mi::hashed_unique<mi::tag<struct FullId>, FlId>;
    using OwndHsh = mi::hashed_non_unique<mi::tag<struct Owned>, Ownd>;
    using GoodInfoContainer = boost::multi_index_container<GoodInfo, mi::indexed_by<FlIdHsh, OwndHsh>>;
    GoodInfoContainer goodsInfo;  // known information about each good by full id
    std::vector<TownInfo> nearby; // known information about nearby towns
    AIRole role;                  // behavior for this ai
    void setNearby(const Town *t, const Town *tT, unsigned int i);
    void setLimits();
    static double buyScore(double p, double b) { return p == 0 ? 0 : b / p; }  // score selling at price p
    static double sellScore(double p, double s) { return s == 0 ? 0 : p / s; } // score buying at price p
    double equipScore(const Good &eq, const EnumArray<unsigned int, Stat> &sts) const;
    double equipScore(const std::vector<Good> &eqpmt, const EnumArray<unsigned int, Stat> &sts) const;
    double equipScore(const Good &eq, const std::vector<Good> &eqpmt, const EnumArray<unsigned int, Stat> &sts) const;
    double lootScore(const Property &ppt);
    void trade();
    void store(const Property *sPpt, const Property &tPpt);
    void equip();
    void attack();

public:
    AI(Traveler &tvl, const EnumArray<double, DecisionCriteria> &dcC,
       const GoodInfoContainer &gsI, AIRole rl);
    AI(Traveler &tvl) : AI(tvl, Settings::aIDecisionCriteria(), {}, Settings::aIRole()) {}
    AI(Traveler &tvl, const AI &p) : AI(tvl, p.decisionCriteria, p.owned, p.watched, p.role) {}
    AI(Traveler &tvl, const Save::AI *ldAI);
    flatbuffers::Offset<Save::AI> save(flatbuffers::FlatBufferBuilder &b) const;
    AIRole getRole() const { return role; }
    void choose();
    void loot();
    void update(unsigned int elTm);
};

#endif // AI_H
