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

#include "save_generated.h"

#include "traveler.hpp"

enum class FightChoice;

class Good;

class Town;

class Traveler;

struct MaterialInfo {
    double limitFactor = 0.; // factor controlling value based on min/max price
    double minPrice = 0.,
           maxPrice = 0.; // minimum and maximum price of material nearby
    double value = 0., buy = 0.,
           sell = 0.; // estimated value, maximum buy price, and minimum sell price
};

struct TownInfo {
    Town *town;                 // pointer to town in question
    double buyScore, sellScore; // highest buy and sell scores for town
};

/**
 * AI which updates non-player travelers
 */
class AI {
    Traveler &traveler;                     // the traveler this AI controls
    int decisionCounter;                    // counter for updatening AI
    std::array<double, 8> decisionCriteria; /* buy/sell score weight, weapon/armor
    equip score, tendency to fight/update/yield, looting greed */
    std::vector<MaterialInfo> materialInfo; // known information about each material of each good
    std::vector<TownInfo> nearby;           // known information about nearby towns
    void randomizeLimitFactors();
    void randomizeCriteria();
    void setNearby(const Town *t, const Town *tT, unsigned int i);
    void setLimits();
    double equipScore(const Good &e, const std::array<unsigned int, 5> &sts) const;
    double equipScore(const std::vector<Good> &eqpmt, const std::array<unsigned int, 5> &sts) const;
    double lootScore(const std::vector<Good> &tGs) const;
    void trade();
    void equip();
    void attack();

public:
    AI(Traveler &tvl);
    AI(Traveler &tvl, const AI &p);
    AI(Traveler &tvl, const Save::AI *a);
    void choose();
    void loot();
    void update(unsigned int e);
    flatbuffers::Offset<Save::AI> save(flatbuffers::FlatBufferBuilder &b) const;
};

#endif // AI_H
