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

#include "ai.hpp"

AI::AI(Traveler &tvl) : traveler(tvl) {
    // Initialize variables for running a new AI based on starting goods and current town.
    role = Settings::aIRole();
    decisionCriteria = Settings::aIDecisionCriteria();
    decisionCounter = Settings::aIDecisionCounter();
    auto toTown = traveler.getTown();
    setNearby(toTown, toTown, Settings::getAITownRange());
    setLimits();
}

AI::AI(Traveler &tvl, const AI &p)
    : traveler(tvl), decisionCriteria(p.decisionCriteria), goodsInfo(p.goodsInfo), role(p.role) {
    // Starts an AI which imitates parameter's AI.
    const Town *toTown = traveler.getTown();
    setNearby(toTown, toTown, Settings::getAITownRange());
    setLimits();
}

AI::AI(Traveler &tvl, const Save::AI *a) : traveler(tvl), decisionCounter(a->decisionCounter()) {
    // Load AI from flatbuffers.
    auto lDecisionCriteria = a->decisionCriteria();
    for (flatbuffers::uoffset_t i = 0; i < decisionCriteria.size(); ++i)
        decisionCriteria[i] = lDecisionCriteria->Get(i);
    auto lGoodsInfo = a->goodsInfo();
    for (auto gII = lGoodsInfo->begin(); gII != lGoodsInfo->end(); ++gII)
        goodsInfo[(*gII)->fullId()] = {(*gII)->limitFactor(), (*gII)->minPrice(), (*gII)->maxPrice(),
                                       (*gII)->estimate(),    (*gII)->buy(),      (*gII)->sell()};
}

flatbuffers::Offset<Save::AI> AI::save(flatbuffers::FlatBufferBuilder &b) const {
    auto sDecisionCriteria = b.CreateVector(std::vector<double>(begin(decisionCriteria), end(decisionCriteria)));
    std::vector<std::pair<unsigned int, GoodInfo>> vGoodsInfo(begin(goodsInfo), end(goodsInfo));
    auto sMaterialInfo =
        b.CreateVectorOfStructs<Save::GoodInfo>(goodsInfo.size(), [vGoodsInfo](size_t i, Save::GoodInfo *gI) {
            *gI = Save::GoodInfo(vGoodsInfo[i].first, vGoodsInfo[i].second.limitFactor, vGoodsInfo[i].second.minPrice,
                                 vGoodsInfo[i].second.maxPrice, vGoodsInfo[i].second.estimate,
                                 vGoodsInfo[i].second.buy, vGoodsInfo[i].second.sell);
        });
    return Save::CreateAI(b, static_cast<short>(decisionCounter), sDecisionCriteria, sMaterialInfo);
}

double AI::equipScore(const Good &e, const std::array<unsigned int, 5> &sts) const {
    // Scores parameter equipment based on parameter stats and this ai's criteria
    double attackScore = 0;
    double defenseScore = 0;
    for (auto &s : e.getCombatStats()) {
        attackScore += s.attack * sts[static_cast<size_t>(s.stat)];
        attackScore *= s.speed * sts[static_cast<size_t>(s.stat)];
        for (auto &d : s.defense) defenseScore += d * sts[static_cast<size_t>(s.stat)];
    }
    return attackScore * decisionCriteria[2] + defenseScore * decisionCriteria[3];
}

double AI::equipScore(const std::vector<Good> &eqpmt, const std::array<unsigned int, 5> &sts) const {
    // Scores parameter equipment based on this traveler's criteria.
    double score = 1;
    for (auto &e : eqpmt) { score += equipScore(e, sts); }
    return score;
}

double AI::lootScore(const Property &tPpt) const {
    // Total value from looting given set of goods.
    double score = 0;
    tPpt.queryGoods([this, &score](const Good &tG) {
        auto gII = goodsInfo.find(tG.getFullId());
        if (gII != end(goodsInfo)) score += tG.getAmount() * gII->second.estimate;
    });
    return score;
}

void AI::trade() {
    // Attempt to make best possible single trade in current town.
    traveler.clearTrade();
    double bestScore = 0, offerValue = 0, offerWeight = 0, weight = traveler.weight();
    bool overWeight = weight > 0;
    std::unique_ptr<Good> bestGood;
    // Find highest sell score.
    traveler.getProperty().queryGoods([this, weight, &bestScore, &offerValue, &offerWeight, overWeight,
                                       &bestGood](const Good &g) {
        double gWgt = g.weight();
        if (!overWeight || gWgt > 0) {
            // Either we are not over weight or given material doesn't help carry.
            auto fId = g.getFullId();
            auto &tG = traveler.getProperty().good(fId);
            auto amount = g.getAmount();
            if (amount > 0) {
                if (gWgt < 0 && weight > gWgt)
                    // This good is needed to carry existing goods, reduce amount.
                    amount *= weight / gWgt;
                auto gII = goodsInfo.find(fId);
                if (gII == end(goodsInfo)) return;
                double score = sellScore(tG.price(), gII->second.sell); // score based on minimum sell price
                if ((overWeight && !bestGood || gWgt > bestGood->weight()) || (score > bestScore)) {
                    // Either we are over weight and good is heavier than previous offer or this good scores better.
                    bestScore = score;
                    if (!g.getSplit()) amount = floor(amount);
                    bestGood = std::make_unique<Good>(tG.getFullId(), tG.getFullName(), amount, tG.getMeasure());
                    offerValue = tG.price(amount);
                    offerWeight = gWgt;
                }
            }
        }
    });
    // If no good found to sell, stop trading.
    if (!bestGood) return;
    // Add best good to offer if found.
    traveler.offer.push_back(*bestGood);
    bestGood = nullptr;
    weight -= offerWeight;
    overWeight = weight > 0;
    // Force a trade to occur if over weight.
    if (overWeight)
        bestScore = 0;
    else
        bestScore = 1 / bestScore;
    double excess, townProfit = Settings::getTownProfit();
    // Find highest buy score.
    traveler.getTown()->getProperty().queryGoods([this, &bestScore, offerValue, weight, overWeight, &bestGood,
                                                  townProfit, &excess](const Good &tG) {
        double carry = tG.getCarry();
        if (!overWeight || carry < 0) {
            auto fId = tG.getFullId();
            auto gII = goodsInfo.find(fId);
            if (gII == end(goodsInfo)) return;
            double score = buyScore(tG.price(), gII->second.buy); // score based on maximum buy price
            if (score > bestScore) {
                bestScore = score;
                // Remove amout town takes as profit, store excess.
                excess = 0;
                double amount = tG.quantity(offerValue * townProfit, excess);
                if (amount > 0) {
                    if (overWeight) {
                        // Try to buy minimum that will bring net weight back below 0
                        double needed = ceil(weight / -carry);
                        if (amount > needed) {
                            excess += amount - needed;
                            amount = needed;
                        }
                    }
                    if (!tG.getSplit())
                        // Remove extra portion of goods that don't split.
                        excess += modf(amount, &amount);
                    // Convert the excess from units of bought good to deniers.
                    excess = tG.price(excess);
                    bestGood = std::make_unique<Good>(tG.getFullId(), tG.getFullName(), amount, tG.getMeasure());
                }
            }
        }
    });
    if (!bestGood) return;
    if (excess > 0) traveler.divideExcess(excess);
    traveler.request.push_back(*bestGood);
    // Make the trade.
    traveler.makeTrade();
}

void AI::equip() {
    // Equip best scoring item for each part.
    std::array<double, static_cast<size_t>(Part::count)> bestScores;
    traveler.getProperty().queryGoods([this, &bestScores](const Good &g) {
        if (g.getAmount() >= 1) {
            auto &ss = g.getCombatStats();
            if (!ss.empty()) {
                Good e(g.getFullId(), g.getFullName(), 1, ss, g.getImage());
                double score = equipScore(e, traveler.getStats());
                Part pt = ss.front().part;
                if (score > bestScores[static_cast<size_t>(pt)]) {
                    bestScores[static_cast<size_t>(pt)] = score;
                    traveler.equip(e);
                }
            }
        }
    });
}

void AI::attack() {
    // Attack traveler with lower equipment score and lootable goods.
    if (!traveler.getTarget()) {
        // There isn't already a target.
        auto able = traveler.attackable();
        double ourEquipScore = equipScore(traveler.getEquipment(), traveler.getStats());
        // Remove travelers who do not meet threshold for attacking.
        able.erase(std::remove_if(begin(able), end(able),
                                  [this, ourEquipScore](const Traveler *a) {
                                      return decisionCriteria[4] < Settings::getCriteriaMax() / 3 ||
                                             lootScore(a->properties.find(0)->second) * ourEquipScore *
                                                     decisionCriteria[4] /
                                                     equipScore(a->getEquipment(), a->getStats()) <
                                                 Settings::getAIAttackThreshold();
                                  }),
                   end(able));
        if (!able.empty()) {
            // Attack the easiest target.
            auto easiest = std::min_element(begin(able), end(able), [this](const Traveler *a, const Traveler *b) {
                return equipScore(a->equipment, a->stats) < equipScore(b->equipment, b->stats);
            });
            traveler.attack(*easiest);
        }
    }
}

void AI::choose() {
    // Choose to fight, update, or yield based on equip scores, stats, and speeds.
    std::array<double, 3> scores; // fight, update, yield scores
    auto target = traveler.getTarget();
    double equipmentScoreRatio = equipScore(target->equipment, target->getStats()) /
                                 equipScore(traveler.getEquipment(), traveler.getStats());
    scores[0] = 1 / equipmentScoreRatio * decisionCriteria[4];
    scores[1] = equipmentScoreRatio * decisionCriteria[5];
    scores[2] = equipmentScoreRatio * decisionCriteria[6];
    if (equipmentScoreRatio > 1) {
        // Target's equipment is better, weigh update and yield decisions by speed
        // ratio.
        double speedRatio = target->speed() / traveler.speed();
        scores[1] /= speedRatio;
        scores[2] *= speedRatio;
    }
    traveler.choice = static_cast<FightChoice>(std::max_element(begin(scores), end(scores)) - begin(scores));
}

void AI::loot() {
    // Automatically loot based on scores and decision criteria.
    auto tgt = traveler.getTarget();
    if (!tgt) return;
    auto &tPpt = tgt->properties.find(0)->second;
    double lootGoal = lootScore(tPpt);
    if (tgt->alive())
        // Looting from an alive target dependent on greed.
        lootGoal *= decisionCriteria[7] / Settings::getCriteriaMax();
    double looted = 0, weight = traveler.weight();
    while (looted < lootGoal) {
        // Keep looting until amount looted matches goal or we can carry no more.
        double bestScore = 0, bestValue, bestWeight;
        std::unique_ptr<Good> bestGood;
        tPpt.queryGoods([this, &bestScore, &bestValue, &bestWeight, &bestGood, looted, lootGoal](const Good &tG) {
            double amount = tG.getAmount();
            if (amount > 0) {
                auto gII = goodsInfo.find(tG.getFullId());
                if (gII == end(goodsInfo)) return;
                double estimate = gII->second.estimate;
                double carry = tG.getCarry();
                double score;
                if (carry > 0)
                    // Heavier goods score lower.
                    score = estimate / carry;
                else if (carry == 0)
                    // Weightless goods score highest.
                    score = std::numeric_limits<double>::max();
                else
                    // Goods that carry other good score negative.
                    score = carry;
                if ((bestScore >= 0 && score > bestScore) || (score < 0 && score < bestScore)) {
                    // Either this is more valuable per weight than current best or this helps carry more than current best.
                    bestScore = score;
                    amount = std::min((lootGoal - looted) / estimate, amount);
                    bestValue = estimate * amount;
                    bestWeight = carry * amount;
                    bestGood = std::make_unique<Good>(tG.getFullId(), amount);
                }
            }
        });
        if (!bestGood) return;
        // Add the weight of looted good to weight variable.
        weight += bestWeight;
        // Stop looting if we would be overweight.
        if (weight > 0) return;
        // Loot the current best good from target.
        traveler.loot(*bestGood);
        looted += bestValue;
    }
}

void AI::setNearby(const Town *t, const Town *tT, unsigned int i) {
    // Recursively adds towns i steps away to nearby towns. Clears buy and sell scores.
    if (t == tT) {
        // Town t is the current town, clear nearby towns.
        nearby.clear();
    }
    if (i)
        // Town t is within i steps of town tT.
        for (auto &nb : t->getNeighbors())
            if (nb != tT && std::find_if(begin(nearby), end(nearby), [nb](TownInfo t) { return t.town == nb; }) ==
                                end(nearby)) {
                // Neighbor n is not the current town and not already in nearby towns.
                nearby.push_back({nb, 0, 0});
                setNearby(nb, tT, i - 1);
            }
}

void AI::setLimits() {
    // Sets buy and sell limits and min/max prices to reflect current nearby towns. Also updates buy and sell scores.
    size_t nearbyCount = nearby.size(); // nearby town count
    for (auto &nb : nearby) {
        // Reset buy/sell scores
        nb.buyScore = 0;
        nb.sellScore = 0;
        // Find minimum and maximum price for each good in nearby towns.
        nb.town->getProperty().queryGoods([this](const Good &nG) {
            auto price = nG.price();
            auto fId = nG.getFullId();
            auto gII = goodsInfo.find(fId);
            if (gII == end(goodsInfo))
                gII = goodsInfo
                          .insert(std::make_pair(
                              fId, GoodInfo{.limitFactor = Settings::aILimitFactor(), .minPrice = price, .maxPrice = price}))
                          .first;
            else {
                gII->second.minPrice = std::min(gII->second.minPrice, price);
                gII->second.maxPrice = std::max(gII->second.maxPrice, price);
            }
        });
    }
    // Set value and buy and sell based on min and max prices.
    for (auto &gI : goodsInfo) {
        gI.second.estimate = gI.second.minPrice + gI.second.limitFactor * (gI.second.maxPrice - gI.second.minPrice);
        gI.second.buy = gI.second.estimate * Settings::getTownProfit();
        gI.second.sell = gI.second.estimate / Settings::getTownProfit();
    }
    auto &ppt = traveler.properties.find(0)->second;
    // Loop through nearby towns again now that info has been gathered to set buy and sell scores.
    for (auto &nb : nearby)
        nb.town->getProperty().queryGoods([this, &ppt, &nb](const Good &nG) {
            auto price = nG.price();
            auto fId = nG.getFullId();
            if (ppt.hasGood(fId) && price > 0)
                // We have the good and it is worth something in the town.
                nb.sellScore = std::max(sellScore(price, goodsInfo[fId].sell), nb.sellScore);
            else
                // Only score buying goods not already owned.
                nb.buyScore = std::max(buyScore(price, goodsInfo[fId].buy), nb.buyScore);
        });
}

void AI::update(unsigned int e) {
    // Run the AI for the elapsed time. Includes trading, equipping, and attacking.
    if (!traveler.getMoving()) {
        decisionCounter += e;
        if (decisionCounter > 0) {
            trade();
            equip();
            attack();
            size_t nearbyCount = nearby.size();
            std::vector<double> townScores(nearbyCount); // scores for going to each town
            // Set scores based on buying and selling prices in each town.
            std::transform(begin(nearby), end(nearby), begin(townScores), [this](const TownInfo &tI) {
                return tI.buyScore * decisionCriteria[0] + tI.sellScore * decisionCriteria[1];
            });
            auto bestTownScore = std::max_element(begin(townScores), end(townScores));
            if (bestTownScore != end(townScores)) {
                traveler.pickTown(nearby[static_cast<size_t>(bestTownScore - begin(townScores))].town);
                const Town *toTown = traveler.getTown();
                setNearby(toTown, toTown, Settings::getAITownRange());
                setLimits();
            }
            decisionCounter -= Settings::getAIDecisionTime();
        }
    }
}
