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
    auto &rng = Settings::getRng();
    auto &rWs = Settings::getAIRoleWeights();
    std::discrete_distribution<> rlDis(begin(rWs), end(rWs));
    role = static_cast<Role>(rlDis(rng));
    // Randomize factors which will be used to choose buy and sell limits for each material.
    auto &gs = traveler.properties.find(0)->second.getGoods();
    // Total count of materials across all goods, including labor.
    goodsInfo = std::vector<GoodInfo>(gs.size());
    static std::uniform_real_distribution<double> lFDis(Settings::getLimitFactorMin(), Settings::getLimitFactorMax());
    for (auto &gI : goodsInfo) gI.limitFactor = lFDis(rng);
    // Randomize decision criteria.
    static std::uniform_real_distribution<double> dcCrtDis(1, Settings::getCriteriaMax());
    for (auto &c : decisionCriteria) c = dcCrtDis(rng);
    // Randomize decision counter.
    static std::uniform_int_distribution<> dcCntDis(-Settings::getAIDecisionTime(), 0);
    decisionCounter = dcCntDis(rng);
    auto toTown = traveler.toTown;
    setNearby(toTown, toTown, Settings::getAITownRange());
    setLimits();
}

AI::AI(Traveler &tvl, const AI &p)
    : traveler(tvl), decisionCriteria(p.decisionCriteria), goodsInfo(p.goodsInfo), role(p.role) {
    // Starts an AI which imitates parameter's AI.
    const Town *toTown = traveler.toTown;
    setNearby(toTown, toTown, Settings::getAITownRange());
    setLimits();
}

AI::AI(Traveler &tvl, const Save::AI *a) : traveler(tvl), decisionCounter(a->decisionCounter()) {
    // Load AI from flatbuffers.
    auto lDecisionCriteria = a->decisionCriteria();
    for (flatbuffers::uoffset_t i = 0; i < decisionCriteria.size(); ++i)
        decisionCriteria[i] = lDecisionCriteria->Get(i);
    auto lMaterialInfo = a->materialInfo();
    for (auto mII = lMaterialInfo->begin(); mII != lMaterialInfo->end(); ++mII)
        goodsInfo.push_back({(*mII)->limitFactor(), (*mII)->minPrice(), (*mII)->maxPrice(), (*mII)->value(),
                             (*mII)->buy(), (*mII)->sell()});
}

flatbuffers::Offset<Save::AI> AI::save(flatbuffers::FlatBufferBuilder &b) const {
    auto sDecisionCriteria = b.CreateVector(std::vector<double>(begin(decisionCriteria), end(decisionCriteria)));
    auto sMaterialInfo = b.CreateVectorOfStructs<Save::MaterialInfo>(goodsInfo.size(), [this](size_t i, Save::MaterialInfo *mI) {
        *mI = Save::MaterialInfo(goodsInfo[i].limitFactor, goodsInfo[i].minPrice, goodsInfo[i].maxPrice,
                                 goodsInfo[i].value, goodsInfo[i].buy, goodsInfo[i].sell);
    });
    return Save::CreateAI(b, static_cast<short>(decisionCounter), sDecisionCriteria, sMaterialInfo);
}

double AI::equipScore(const Good &e, const std::array<unsigned int, 5> &sts) const {
    // Scores parameter equipment based on parameter stats and this ai's criteria
    double attackScore = 0;
    double defenseScore = 0;
    for (auto &s : e.getCombatStats()) {
        attackScore += s.attack * sts[s.statId];
        attackScore *= s.speed * sts[s.statId];
        for (auto &d : s.defense) defenseScore += d * sts[s.statId];
    }
    return attackScore * decisionCriteria[2] + defenseScore * decisionCriteria[3];
}

double AI::equipScore(const std::vector<Good> &eqpmt, const std::array<unsigned int, 5> &sts) const {
    // Scores parameter equipment based on this traveler's criteria.
    double score = 1;
    for (auto &e : eqpmt) { score += equipScore(e, sts); }
    return score;
}

double AI::lootScore(const std::vector<Good> &tGs) const {
    // Total value from looting given set of goods.
    double score = 0;
    auto gII = begin(goodsInfo); // material info iterator
    for (auto &tG : tGs) score += tG.getAmount() * gII++->value;
    return score;
}

void AI::trade() {
    // Attempt to make best possible single trade in current town.
    traveler.clearTrade();
    double amount, score, offerValue = 0, weight = traveler.weight(), offerWeight = 0, bestScore = 0;
    bool overWeight = weight > 0;
    std::unique_ptr<Good> bestGood;
    // Find highest sell score.
    auto gII = begin(goodsInfo); // material info iterator
    auto &gs = traveler.properties.find(0)->second.getGoods();
    auto &tGs = traveler.toTown->getProperty().getGoods();
    for (auto &g : gs) {
        double gWgt = g.weight();
        if (!overWeight || gWgt > 0) {
            // Either we are not over weight or given material doesn't help carry.
            auto &tG = tGs[g.getFullId()];
            amount = g.getAmount();
            if (amount > 0) {
                if (gWgt < 0 && weight > gWgt)
                    // This good is needed to carry existing goods, reduce amount.
                    amount *= weight / gWgt;
                if (amount <= 0) continue;
                score = sellScore(tG.price(), gII->sell); // score based on minimum sell price
                if ((overWeight && gWgt > offerWeight) || (score > bestScore)) {
                    // Either we are over weight and good is heavier than previous offer or this good scores better.
                    bestScore = score;
                    if (!g.getSplit()) amount = floor(amount);
                    bestGood = std::make_unique<Good>(tG.getFullId(), tG.getFullName(), amount, tG.getMeasure());
                    offerValue = tG.price(amount);
                    offerWeight = gWgt;
                }
            }
        }
        ++gII;
    }
    // If no good found, stop trading.
    if (!bestGood) return;
    // Add best good to offer if found.
    traveler.offer.push_back(*bestGood);
    bestGood = nullptr;
    // Force a trade to occur if over weight.
    if (overWeight)
        bestScore = 0;
    else
        bestScore = 1 / bestScore;
    double excess, townProfit = Settings::getTownProfit();
    amount = 0;
    // Find highest buy score.
    gII = begin(goodsInfo); // material info iterator
    // Loop through goods.
    for (auto &tG : tGs) {
        double carry = tG.getCarry();
        if (!overWeight || carry < 0) {
            score = buyScore(tG.price(), gII->buy); // score based on maximum buy price
            if (score > bestScore) {
                bestScore = score;
                // Remove amout town takes as profit, store excess.
                excess = 0;
                amount = tG.quantity(offerValue * townProfit, excess);
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
        ++gII;
    }
    if (!bestGood) return;
    if (excess > 0) traveler.divideExcess(excess);
    traveler.request.push_back(*bestGood);
    // Make the trade.
    traveler.makeTrade();
}

void AI::equip() {
    // Equip best scoring item for each part.
    std::array<double, 6> bestScore;
    auto &gs = traveler.properties.find(0)->second.getGoods();
    for (auto &g : gs)
        if (g.getAmount() >= 1) {
            auto &ss = g.getCombatStats();
            if (!ss.empty()) {
                Good e(g.getFullId(), g.getFullName(), 1, ss, g.getImage());
                double score = equipScore(e, traveler.stats);
                unsigned int pId = ss.front().partId;
                if (score > bestScore[pId]) {
                    bestScore[pId] = score;
                    traveler.equip(e);
                }
            }
        }
}

void AI::attack() {
    // Attack traveler with lower equipment score and lootable goods.
    if (!traveler.target) {
        // There isn't already a target.
        auto able = traveler.attackable();
        double ourEquipScore = equipScore(traveler.equipment, traveler.stats);
        // Remove travelers who do not meet threshold for attacking.
        able.erase(std::remove_if(begin(able), end(able),
                                  [this, ourEquipScore](const Traveler *a) {
                                      return decisionCriteria[4] < Settings::getCriteriaMax() / 3 ||
                                             lootScore(a->properties.find(0)->second.getGoods()) * ourEquipScore * decisionCriteria[4] /
                                                     equipScore(a->equipment, a->stats) <
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
    auto target = traveler.target;
    double equipmentScoreRatio =
        equipScore(target->equipment, target->getStats()) / equipScore(traveler.equipment, traveler.stats);
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
    auto tgt = traveler.target;
    if (!tgt) return;
    auto &tGs = tgt->properties.find(0)->second.getGoods();
    double lootGoal = lootScore(tGs);
    if (tgt->alive())
        // Looting from an alive target dependent on greed.
        lootGoal *= decisionCriteria[7] / Settings::getCriteriaMax();
    double looted = 0; // value that has already been looted
    double weight = traveler.weight();
    while (looted < lootGoal) {
        // Keep looting until amount looted matches goal or we can carry no more.
        std::unique_ptr<Good> bestGood;
        double bestScore = 0;
        double bestLooted = 0;
        double bestWeight = 0;
        auto gII = begin(goodsInfo); // material info iterator
        for (auto &tG : tGs) {
            double amount = tG.getAmount();
            if (amount > 0) {
                double value = gII->value;
                double carry = tG.getCarry();
                double score;
                if (carry > 0)
                    // Heavier goods score lower.
                    score = value / carry;
                else if (carry == 0)
                    // Weightless goods score highest.
                    score = std::numeric_limits<double>::max();
                else
                    // Goods that carry other good score negative.
                    score = carry;
                if ((bestScore >= 0 && score > bestScore) || (score < 0 && score < bestScore)) {
                    // Either this is more valuable per weight than current best or this helps carry more than current best.
                    bestScore = score;
                    if (looted + amount * value > lootGoal) amount = (lootGoal - looted) / value;
                    bestLooted = amount * value;
                    bestGood = std::make_unique<Good>(tG.getFullId(), amount);
                    bestWeight = amount * carry;
                }
            }
            ++gII;
        }
        // Add the weight of looted good to weight variable.
        weight += bestWeight;
        if (!bestGood || weight > 0) break;
        looted += bestLooted;
        // Loot the current best good from target.
        traveler.loot(*bestGood);
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
        for (auto &n : t->getNeighbors())
            if (n != tT && std::find_if(begin(nearby), end(nearby), [n](TownInfo t) { return t.town == n; }) == end(nearby)) {
                // Neighbor n is not the current town and not already in nearby towns.
                nearby.push_back({n, 0, 0});
                setNearby(n, tT, i - 1);
            }
}

void AI::setLimits() {
    // Sets buy and sell limits and min/max prices to reflect current nearby towns. Also updates buy and sell scores.
    size_t nC = nearby.size(); // nearby town count
    // Reset buy/sell scores
    for (auto &n : nearby) {
        n.buyScore = 0;
        n.sellScore = 0;
    }
    auto &gs = traveler.properties.find(0)->second.getGoods();
    size_t gC = gs.size();       // good count
    auto gII = begin(goodsInfo); // material info iterator
    // Find minimum and maximum price for each material of each good in nearby towns.
    for (auto &g : gs) {
        // Loop through goods.
        bool hasGood = g.getAmount() > 0; // Traveler has non-zero amount of material j
        std::vector<double> prices;            // prices for this material of this good in
                                                // all nearby towns that sell it
        prices.reserve(nC);
        for (auto &n : nearby) {
            // Loop through nearby towns to collect price info.
            auto &nG = n.town->getProperty().getGoods()[g.getFullId()];
            double price = nG.price();
            if (price > 0)
                // Good i material j is sold in nearby town k.
                prices.push_back(price);
        }
        if (!prices.empty()) {
            auto mMP = std::minmax_element(begin(prices), end(prices));
            gII->minPrice = *mMP.first;
            gII->maxPrice = *mMP.second;
            gII->value = *mMP.first + gII->limitFactor * (*mMP.second - *mMP.first);
            gII->buy = gII->value * Settings::getTownProfit();
            gII->sell = gII->value / Settings::getTownProfit();
        }
        for (auto &n : nearby) {
            // Loop through nearby towns again now that material info has been gathered.
            auto &nG = n.town->getProperty().getGoods()[g.getFullId()];
            double price = nG.price();
            if (hasGood && price > 0)
                n.sellScore = std::max(sellScore(price, gII->sell), n.sellScore);
            else
                // Only score buying materials not already owned.
                n.buyScore = std::max(buyScore(price, gII->buy), n.buyScore);
        }
        ++gII;
    }
}

void AI::update(unsigned int e) {
    // Run the AI for the elapsed time. Includes trading, equipping, and attacking.
    if (!traveler.moving) {
        decisionCounter += e;
        if (decisionCounter > 0) {
            trade();
            equip();
            attack();
            size_t nC = nearby.size();
            std::vector<double> townScores(nC); // scores for going to each town
            // Set scores based on buying and selling prices in each town.
            std::transform(begin(nearby), end(nearby), begin(townScores), [this](const TownInfo &tI) {
                return tI.buyScore * decisionCriteria[0] + tI.sellScore * decisionCriteria[1];
            });
            auto bestTownScore = std::max_element(begin(townScores), end(townScores));
            if (bestTownScore != end(townScores)) {
                traveler.pickTown(nearby[static_cast<size_t>(bestTownScore - begin(townScores))].town);
                const Town *toTown = traveler.toTown;
                setNearby(toTown, toTown, Settings::getAITownRange());
                setLimits();
            }
            decisionCounter -= Settings::getAIDecisionTime();
        }
    }
}
