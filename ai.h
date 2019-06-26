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

#include "traveler.h"

enum class FightChoice;

class Good;

class Town;

class Traveler;

struct MaterialInfo {
    double limitFactor = 0;              // factor controlling value based on min/max price
    double minPrice = 0, maxPrice = 0;   // minimum and maximum price of material nearby
    double value = 0, buy = 0, sell = 0; // estimated value, maximum buy price, and minimum sell price
};

struct TownInfo {
    Town *town;                 // pointer to town in question
    double buyScore, sellScore; // highest buy and sell scores for town
};

/**
 * AI which runs non-player travelers
 */
class AI {
    int decisionCounter;                    // counter for running AI
    std::array<double, 8> decisionCriteria; /* buy/sell score weight, weapon/armor equip score, tendency to fight/run/yield,
    looting greed */
    std::vector<MaterialInfo> materialInfo; // known information about each material of each good
    std::vector<TownInfo> nearby;           // known information about nearby towns
    void randomizeLimitFactors(const std::vector<Good> &gs);
    void randomizeCriteria();
    void setNearby(const Town *t, const Town *tT, unsigned int i);
    void setLimits(const std::vector<Good> &gs);
    double equipScore(const Good &e, const std::array<unsigned int, 5> &sts) const;
    double equipScore(const std::vector<Good> &eqpmt, const std::array<unsigned int, 5> &sts) const;
    double lootScore(const std::vector<Good> &gs, const std::vector<Good> &tGs) const;
    void autoTrade(std::vector<Good> &o, std::vector<Good> &r, const std::vector<Good> &gs, const Town *tT,
                   const std::function<double()> &netWgt, const std::function<void()> &mT);
    void autoEquip(const std::vector<Good> &gs, const std::array<unsigned int, 5> &sts,
                   const std::function<void(Good &e)> &eqp);
    void autoAttack(const std::weak_ptr<Traveler> tgt, const std::function<std::vector<std::shared_ptr<Traveler>>()> &atkabl,
                    const std::vector<Good> &gs, const std::vector<Good> &eqpmt, const std::array<unsigned int, 5> &sts,
                    const std::function<void(std::shared_ptr<Traveler>)> &atk);

  public:
    AI(const std::vector<Good> &gs, Town *tT);
    AI(const AI &p, const std::vector<Good> &gs, Town *tT);
    AI(const Save::AI *a);
    void autoChoose(const std::vector<Good> &gs, const std::array<unsigned int, 5> &sts,
                    const std::function<unsigned int()> &spd, const std::vector<Good> &tGs,
                    const std::array<unsigned int, 5> &tSts, const std::function<unsigned int()> &tSpd, FightChoice &choice);
    void autoLoot(const std::vector<Good> &gs, std::weak_ptr<Traveler> tgt, const std::function<double()> &netWgt,
                  const std::function<void(Good &g, Traveler &t)> &lt);
    void run(unsigned int e, bool m, std::vector<Good> &o, std::vector<Good> &r, const std::vector<Good> &gs,
             const std::vector<Good> &eqpmt, const Town *tT, const std::weak_ptr<Traveler> tgt,
             const std::array<unsigned int, 5> &sts, const std::function<double()> &netWgt, const std::function<void()> &mT,
             const std::function<void(Good &e)> &eqp, const std::function<std::vector<std::shared_ptr<Traveler>>()> &atkabl,
             const std::function<void(std::shared_ptr<Traveler>)> &atk, const std::function<void(Town *)> pT);
    flatbuffers::Offset<Save::AI> save(flatbuffers::FlatBufferBuilder &b);
};

#endif // AI_H
