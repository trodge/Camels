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

#include "ai.h"

AI::AI(const std::vector<Good> &gs, Town *tT) {
    // Initialize variables for running a new AI based on starting goods and current town.
    randomizeLimitFactors(gs);
    randomizeCriteria();
    setNearby(tT, tT, Settings::getAITownRange());
    setLimits(gs);
}

AI::AI(const AI &p, const std::vector<Good> &gs, Town *tT) {
    // Starts an AI which imitates parameter's AI.
    materialInfo = p.materialInfo;
    setNearby(tT, tT, Settings::getAITownRange());
    setLimits(gs);
}

AI::AI(const Save::AI *a) : decisionCounter(a->decisionCounter()) {
    auto lDecisionCriteria = a->decisionCriteria();
    for (unsigned int i = 0; i < decisionCriteria.size(); ++i)
        decisionCriteria[i] = lDecisionCriteria->Get(i);
    auto lMaterialInfo = a->materialInfo();
    for (auto mII = lMaterialInfo->begin(); mII != lMaterialInfo->end(); ++mII)
        materialInfo.push_back(
            {(*mII)->limitFactor(), (*mII)->minPrice(), (*mII)->maxPrice(), (*mII)->value(), (*mII)->buy(), (*mII)->sell()});
}

void AI::randomizeLimitFactors(const std::vector<Good> &gs) {
    // Randomize factors which will be used to choose buy and sell limits for each material.
    size_t mC = 0; // Total count of materials across all goods, excluding labor.
    for (auto gI = gs.begin() + 1; gI != gs.end(); ++gI)
        mC += gI->getMaterials().size();
    materialInfo = std::vector<MaterialInfo>(mC);
    static std::uniform_real_distribution<> dis(Settings::getLimitFactorMin(), Settings::getLimitFactorMax());
    auto mII = materialInfo.begin();
    for (size_t i = 1; i < gs.size(); ++i) {
        mC = gs[i].getMaterials().size();
        for (size_t j = 0; j < mC; ++j) {
            mII->limitFactor = dis(*Settings::getRng());
            ++mII;
        }
    }
}

void AI::randomizeCriteria() {
    // Randomize AI decision criteria.
    static std::uniform_real_distribution<> rDis(1, Settings::getCriteriaMax());
    for (auto &c : decisionCriteria)
        c = rDis(*Settings::getRng());
    // Randomize decision counter.
    static std::uniform_int_distribution<> iDis(0, Settings::getAIDecisionTime());
    decisionCounter = iDis(*Settings::getRng());
}

double AI::equipScore(const Good &e, const std::array<int, 5> &sts) const {
    // Scores parameter equipment based on parameter stats and this traveler's criteria
    double attackScore = 0;
    double defenseScore = 0;
    for (auto &s : e.getMaterial().getCombatStats()) {
        attackScore += s.attack * sts[s.statId];
        attackScore *= s.speed * sts[s.statId];
        for (auto &d : s.defense)
            defenseScore += d * sts[s.statId];
    }
    return attackScore * decisionCriteria[2] + defenseScore * decisionCriteria[3];
}

double AI::equipScore(const std::vector<Good> &eqpmt, const std::array<int, 5> &sts) const {
    // Scores parameter equipment based on this traveler's criteria.
    double score = 1;
    for (auto &e : eqpmt) {
        score += equipScore(e, sts);
    }
    return score;
}

double AI::lootScore(const std::vector<Good> &gs, const std::vector<Good> &tGs) const {
    // Score for looting target parameter goods given our goods
    double score = 0;
    auto mII = materialInfo.begin(); // material info iterator
    for (size_t i = 1; i < gs.size(); ++i) {
        auto &gMs = gs[i].getMaterials();
        auto &tGMs = tGs[i].getMaterials();
        for (size_t j = 0; j < gMs.size(); ++j) {
            score += tGMs[j].getAmount() * mII->value;
            ++mII;
        }
    }
    return score;
}

void AI::autoTrade(std::vector<Good> &o, std::vector<Good> &r, const std::vector<Good> &gs, const Town *tT,
                   const std::function<double()> &netWgt, const std::function<void()> &mT) {
    // Attempt to make best possible single trade in current town.
    double amount, score, offerValue = 0;
    o.clear();
    r.clear();
    auto &tGs = tT->getGoods();
    double weight = netWgt(), offerWeight = 0;
    bool overWeight = weight > 0;
    double bestScore = 0;
    std::unique_ptr<Good> bestGood;
    // Find highest sell score.
    auto mII = materialInfo.begin(); // material info iterator
    // Loop through goods excluding labor.
    for (auto gI = gs.begin() + 1; gI != gs.end(); ++gI) {
        double carry = gI->getCarry();
        if (not overWeight or carry > 0) {
            // Can carry given good.
            auto &gMs = gI->getMaterials();
            for (auto mI = gMs.begin(); mI != gMs.end(); ++mI) {
                auto &tM = tGs[gI->getId()].getMaterial(*mI);
                amount = mI->getAmount();
                if (amount) {
                    if (carry < 0 and weight > carry * amount)
                        // This good is needed to carry existing goods, reduce amount.
                        amount = weight / carry;
                    if (amount <= 0)
                        continue;
                    score = mI->getSellScore(tM.getPrice(), mII->sell); // score based on minimum sell price
                    if ((not overWeight and score > bestScore) or (overWeight and carry * amount > offerWeight)) {
                        bestScore = score;
                        if (not gI->getSplit())
                            amount = floor(amount);
                        bestGood = std::make_unique<Good>(gI->getId(), gI->getName(), amount, gI->getMeasure());
                        bestGood->addMaterial(Material(mI->getId(), mI->getName(), amount));
                        offerValue = tM.getPrice(amount);
                        offerWeight = amount * carry;
                    }
                }
                ++mII;
            }
        }
    }
    // If no good found, stop trading.
    if (not bestGood)
        return;
    // Add best good to offer if found.
    o.push_back(*bestGood);
    bestGood = nullptr;
    // Force a trade to occur if over weight.
    if (overWeight)
        bestScore = 0;
    else
        bestScore = 1 / bestScore;
    double excess;
    amount = 0;
    // Find highest buy score.
    mII = materialInfo.begin(); // material info iterator
    // Loop through goods excluding labor.
    for (auto gI = tGs.begin() + 1; gI != tGs.end(); ++gI) {
        double carry = gI->getCarry();
        if (not overWeight or carry < 0) {
            auto &gMs = gI->getMaterials();
            for (auto mI = gMs.begin(); mI != gMs.end(); ++mI) {
                auto &rM = gs[gI->getId()].getMaterial(*mI);
                score = rM.getBuyScore(mI->getPrice(), mII->buy); // score based on maximum buy price
                ++mII;
                if (score > bestScore) {
                    bestScore = score;
                    // Remove amout town takes as profit, store excess.
                    excess = 0;
                    amount = mI->getQuantity(offerValue * Settings::getTownProfit(), &excess);
                    if (amount) {
                        if (overWeight) {
                            // Try to buy minimum that will bring net weight back below 0
                            double needed = ceil(weight / -carry);
                            if (amount > needed) {
                                excess += amount - needed;
                                amount = needed;
                            }
                        }
                        if (not gI->getSplit())
                            // Remove extra portion of goods that don't split.
                            excess += modf(amount, &amount);
                        // Convert the excess from units of bought good to deniers.
                        excess = mI->getPrice(excess);
                        bestGood = std::make_unique<Good>(gI->getId(), gI->getName(), amount, gI->getMeasure());
                        bestGood->addMaterial(Material(mI->getId(), mI->getName(), amount));
                    }
                }
            }
        }
    }
    if (not bestGood)
        return;
    if (excess) {
        // Convert excess value to quantity of offered good.
        auto &tM = tGs[o.back().getId()].getMaterial(o.back().getMaterial());
        double q = tM.getQuantity(excess / Settings::getTownProfit());
        if (not gs[o.back().getId()].getSplit())
            // Good can't split, floor excess quantity
            q = floor(q);
        // Remove quantity from offer.
        o.back().use(q);
    }
    r.push_back(*bestGood);
    // Make the trade.
    mT();
}

void AI::autoEquip(const std::vector<Good> &gs, const std::array<int, 5> &sts, const std::function<void(Good &e)> &eqp) {
    // Equip best scoring item for each part.
    std::array<double, 6> bestScore{};
    for (auto &g : gs)
        if (g.getAmount() >= 1)
            for (auto &m : g.getMaterials())
                if (m.getAmount() >= 1) {
                    auto &ss = m.getCombatStats();
                    if (not ss.empty()) {
                        Good e(g.getId(), g.getName(), 1);
                        Material eM(m.getId(), m.getName(), 1);
                        eM.setCombatStats(ss);
                        e.addMaterial(eM);
                        double score = equipScore(e, sts);
                        int pId = ss.front().partId;
                        if (score > bestScore[pId]) {
                            bestScore[pId] = score;
                            eqp(e);
                        }
                    }
                }
}

void AI::autoAttack(const std::weak_ptr<Traveler> tgt, const std::function<std::vector<std::shared_ptr<Traveler>>()> &atkabl,
                    const std::vector<Good> &gs, const std::vector<Good> &eqpmt, const std::array<int, 5> &sts,
                    const std::function<void(std::shared_ptr<Traveler>)> &atk) {
    // Attack traveler with lower equipment score and lootable goods.
    if (not tgt.lock()) {
        // There isn't already a target.
        auto able = atkabl();
        double ourEquipScore = equipScore(eqpmt, sts);
        // Remove travelers who do not meet threshold for attacking.
        able.erase(std::remove_if(able.begin(), able.end(),
                                  [this, ourEquipScore, &gs](const std::shared_ptr<Traveler> a) {
                                      return decisionCriteria[4] < Settings::getCriteriaMax() / 3 or
                                             lootScore(gs, a->getGoods()) * ourEquipScore * decisionCriteria[4] /
                                                     equipScore(a->getEquipment(), a->getStats()) <
                                                 Settings::getAIAttackThreshold();
                                  }),
                   able.end());
        if (not able.empty()) {
            // Attack the easiest target.
            auto easiest = *std::min_element(
                able.begin(), able.end(), [this](const std::shared_ptr<Traveler> a, const std::shared_ptr<Traveler> b) {
                    return equipScore(a->getEquipment(), a->getStats()) < equipScore(b->getEquipment(), b->getStats());
                });
            atk(easiest);
        }
    }
}

void AI::autoChoose(const std::vector<Good> &gs, const std::array<int, 5> &sts, const std::function<int()> &spd,
                    const std::vector<Good> &tGs, const std::array<int, 5> &tSts, const std::function<int()> &tSpd,
                    FightChoice &choice) {
    // Choose to fight, run, or yield based on equip scores, stats, and speeds.
    std::array<double, 3> scores; // fight, run, yield scores
    double equipmentScoreRatio = equipScore(tGs, tSts) / equipScore(gs, sts);
    scores[0] = 1 / equipmentScoreRatio * decisionCriteria[4];
    scores[1] = equipmentScoreRatio * decisionCriteria[5];
    scores[2] = equipmentScoreRatio * decisionCriteria[6];
    if (equipmentScoreRatio > 1) {
        // Weigh run and yield decisions by speed ratio if target's equipment is better.
        double speedRatio = tSpd() / spd();
        scores[1] /= speedRatio;
        scores[2] *= speedRatio;
    }
    choice = FightChoice(std::max_element(scores.begin(), scores.end()) - scores.begin());
}

void AI::autoLoot(const std::vector<Good> &gs, std::weak_ptr<Traveler> tgt, const std::function<double()> &netWgt,
                  const std::function<void(Good &g, Traveler &t)> &lt) {
    // Automatically loot based on scores and decision criteria
    auto t = tgt.lock();
    if (not t)
        return;
    double lootGoal = lootScore(gs, t->getGoods());
    if (t->alive())
        // Looting from an alive target dependent on greed.
        lootGoal *= decisionCriteria[7] / Settings::getCriteriaMax();
    double looted = 0; // value that has already been looted
    while (looted < lootGoal) {
        // Keep looting until amount looted matches goal.
        std::unique_ptr<Good> bestGood;
        double bestScore = 0;
        auto mII = materialInfo.begin(); // material info iterator
        for (unsigned int i = 1; i < gs.size(); ++i) {
            auto &gMs = gs[i].getMaterials();
            auto &tG = t->getGood(i);
            auto &tGMs = tG.getMaterials();
            for (unsigned int j = 0; j < gMs.size(); ++j) {
                auto &tGM = tGMs[j];
                double score = tGM.getAmount() * mII->value / tG.getCarry();
                if ((bestScore >= 0 and score > bestScore) or (score < 0 and score < bestScore)) {
                    bestScore = score;
                    bestGood = std::make_unique<Good>(i, tGM.getAmount());
                    bestGood->addMaterial(Material(j, tGM.getAmount()));
                }
                ++mII;
            }
        }
        if (not bestGood or netWgt() + bestGood->getCarry() * bestGood->getAmount() > 0)
            break;
        looted += bestScore * gs[bestGood->getId()].getCarry();
        // Loot the current best good from target.
        lt(*bestGood, *t);
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
            if (n != tT and
                std::find_if(nearby.begin(), nearby.end(), [n](TownInfo t) { return t.town == n; }) == nearby.end()) {
                // Neighbor n is not the current town and not already in nearby towns.
                nearby.push_back({n, 0, 0});
                setNearby(n, tT, i - 1);
            }
}

void AI::setLimits(const std::vector<Good> &gs) {
    // Sets buy and sell limits and min/max prices to reflect current nearby towns. Also updates buy and sell scores.
    size_t nC = nearby.size(); // nearby town count
    // Reset buy/sell scores
    for (auto &n : nearby) {
        n.buyScore = 0;
        n.sellScore = 0;
    }
    size_t gC = gs.size();           // good count
    auto mII = materialInfo.begin(); // material info iterator
    // Find minimum and maximum price for each material of each good in nearby towns.
    for (size_t i = 1; i < gC; ++i) {
        // Loop through goods.
        size_t mC = gs[i].getMaterials().size(); // materials count
        for (size_t j = 0; j < mC; ++j) {
            // Loop through materials.
            auto &gM = gs[i].getMaterial(j);
            bool hasMaterial = gM.getAmount(); // Traveler has non-zero amount of material j
            std::vector<double> prices;        // prices for this material of this good in all nearby towns that sell it
            prices.reserve(nC);
            for (auto &n : nearby) {
                // Loop through nearby towns to collect price info.
                auto &nM = n.town->getGood(i).getMaterial(j);
                double price = nM.getPrice();
                if (price)
                    // Good i material j is sold in nearby town k.
                    prices.push_back(price);
            }
            if (not prices.empty()) {
                auto mMP = std::minmax_element(prices.begin(), prices.end());
                mII->minPrice = *mMP.first;
                mII->maxPrice = *mMP.second;
                mII->value = *mMP.first + mII->limitFactor * (*mMP.second - *mMP.first);
                mII->buy = mII->value * Settings::getTownProfit();
                mII->sell = mII->value / Settings::getTownProfit();
            }
            for (auto &n : nearby) {
                // Loop through nearby towns again now that material info has been gathered
                auto &nM = n.town->getGood(i).getMaterial(j);
                double price = nM.getPrice();
                if (hasMaterial and price) {
                    double sellScore = gM.getSellScore(price, mII->sell);
                    if (sellScore > n.sellScore)
                        n.sellScore = sellScore;
                } else {
                    // Currently only scores buying materials not already owned.
                    double buyScore = gM.getBuyScore(price, mII->buy);
                    if (buyScore > n.buyScore)
                        n.buyScore = buyScore;
                }
            }
            ++mII;
        }
    }
}

void AI::run(int e, bool m, std::vector<Good> &o, std::vector<Good> &r, const std::vector<Good> &gs,
             const std::vector<Good> &eqpmt, const Town *tT, const std::weak_ptr<Traveler> tgt,
             const std::array<int, 5> &sts, const std::function<double()> &netWgt, const std::function<void()> &mT,
             const std::function<void(Good &e)> &eqp, const std::function<std::vector<std::shared_ptr<Traveler>>()> &atkabl,
             const std::function<void(std::shared_ptr<Traveler>)> &atk, const std::function<void(Town *)> pT) {
    // Run the AI for the elapsed time. Includes trading, equipping, and attacking.
    if (not m) {
        decisionCounter += e;
        if (decisionCounter > Settings::getAIDecisionTime()) {
            autoTrade(o, r, gs, tT, netWgt, mT);
            autoEquip(gs, sts, eqp);
            autoAttack(tgt, atkabl, gs, eqpmt, sts, atk);
            size_t nC = nearby.size();
            std::vector<double> townScores(nC); // scores for going to each town
            // Set scores based on buying and selling prices in each town.
            std::transform(nearby.begin(), nearby.end(), townScores.begin(), [this](const TownInfo &tI) {
                return tI.buyScore * decisionCriteria[0] + tI.sellScore * decisionCriteria[1];
            });
            auto bestTownScore = std::max_element(townScores.begin(), townScores.end());
            if (bestTownScore != townScores.end()) {
                pT(nearby[bestTownScore - townScores.begin()].town);
                setNearby(tT, tT, Settings::getAITownRange());
                setLimits(gs);
            }
            decisionCounter -= Settings::getAIDecisionTime();
        }
    }
}

flatbuffers::Offset<Save::AI> AI::save(flatbuffers::FlatBufferBuilder &b) {
    auto sDecisionCriteria = b.CreateVector(std::vector<float>(decisionCriteria.begin(), decisionCriteria.end()));
    auto sMaterialInfo =
        b.CreateVectorOfStructs<Save::MaterialInfo>(materialInfo.size(), [this](size_t i, Save::MaterialInfo *mI) {
            *mI = Save::MaterialInfo(materialInfo[i].limitFactor, materialInfo[i].minPrice, materialInfo[i].maxPrice,
                                     materialInfo[i].value, materialInfo[i].buy, materialInfo[i].sell);
        });
    return Save::CreateAI(b, static_cast<short>(decisionCounter), sDecisionCriteria, sMaterialInfo);
}
