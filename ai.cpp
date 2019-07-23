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
    // Initialize variables for running a new AI based on starting goods and
    // current town.
    randomizeLimitFactors();
    randomizeCriteria();
    const Town *toTown = traveler.getTown();
    setNearby(toTown, toTown, Settings::getAITownRange());
    setLimits();
}

AI::AI(Traveler &tvl, const AI &p) : traveler(tvl), decisionCriteria(p.decisionCriteria), materialInfo(p.materialInfo) {
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
    auto lMaterialInfo = a->materialInfo();
    for (auto mII = lMaterialInfo->begin(); mII != lMaterialInfo->end(); ++mII)
        materialInfo.push_back({(*mII)->limitFactor(), (*mII)->minPrice(), (*mII)->maxPrice(), (*mII)->value(),
                                (*mII)->buy(), (*mII)->sell()});
}

void AI::randomizeLimitFactors() {
    // Randomize factors which will be used to choose buy and sell limits for each
    // material.
    auto &gs = traveler.getGoods();
    // Total count of materials across all goods, including labor.
    size_t mC = std::accumulate(gs.begin(), gs.end(), 0u,
                                [](unsigned int c, const Good &g) { return c + g.getMaterials().size(); });
    materialInfo = std::vector<MaterialInfo>(mC);
    static std::uniform_real_distribution<double> dis(Settings::getLimitFactorMin(), Settings::getLimitFactorMax());
    auto mII = materialInfo.begin();
    for (size_t i = 0; i < gs.size(); ++i) {
        mC = gs[i].getMaterials().size();
        for (size_t j = 0; j < mC; ++j) {
            mII->limitFactor = dis(Settings::getRng());
            ++mII;
        }
    }
}

void AI::randomizeCriteria() {
    // Randomize AI decision criteria.
    static std::uniform_real_distribution<double> rDis(1, Settings::getCriteriaMax());
    for (auto &c : decisionCriteria) c = rDis(Settings::getRng());
    // Randomize decision counter.
    static std::uniform_int_distribution<int> iDis(0, Settings::getAIDecisionTime());
    decisionCounter = iDis(Settings::getRng());
}

double AI::equipScore(const Good &e, const std::array<unsigned int, 5> &sts) const {
    // Scores parameter equipment based on parameter stats and this ai's criteria
    double attackScore = 0.;
    double defenseScore = 0.;
    for (auto &s : e.getMaterial().getCombatStats()) {
        attackScore += s.attack * sts[s.statId];
        attackScore *= s.speed * sts[s.statId];
        for (auto &d : s.defense) defenseScore += d * sts[s.statId];
    }
    return attackScore * decisionCriteria[2] + defenseScore * decisionCriteria[3];
}

double AI::equipScore(const std::vector<Good> &eqpmt, const std::array<unsigned int, 5> &sts) const {
    // Scores parameter equipment based on this traveler's criteria.
    double score = 1.;
    for (auto &e : eqpmt) { score += equipScore(e, sts); }
    return score;
}

double AI::lootScore(const std::vector<Good> &tGs) const {
    // Score for looting target parameter goods given our goods
    auto &gs = traveler.getGoods();
    double score = 0.;
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

void AI::autoTrade() {
    // Attempt to make best possible single trade in current town.
    double amount, score, offerValue = 0;
    traveler.clearTrade();
    auto &gs = traveler.getGoods();
    auto &tGs = traveler.getTown()->getGoods();
    double weight = traveler.netWeight(), offerWeight = 0.;
    bool overWeight = weight > 0;
    double bestScore = 0;
    std::unique_ptr<Good> bestGood;
    // Find highest sell score.
    auto mII = materialInfo.begin(); // material info iterator
    // Loop through goods excluding labor.
    for (auto gI = gs.begin() + 1; gI != gs.end(); ++gI) {
        double carry = gI->getCarry();
        if (!overWeight || carry > 0.) {
            // Can carry given good.
            auto &gMs = gI->getMaterials();
            for (auto mI = gMs.begin(); mI != gMs.end(); ++mI) {
                auto &tM = tGs[gI->getId()].getMaterial(*mI);
                amount = mI->getAmount();
                if (amount > 0.) {
                    if (carry < 0 && weight > carry * amount)
                        // This good is needed to carry existing goods, reduce amount.
                        amount = weight / carry;
                    if (amount <= 0) continue;
                    score = mI->getSellScore(tM.getPrice(), mII->sell); // score based on minimum sell price
                    if ((overWeight && carry * amount > offerWeight) || (score > bestScore)) {
                        // Either we are over weight and good is heavier than previous offer
                        // or this good scores better.
                        bestScore = score;
                        if (!gI->getSplit()) amount = floor(amount);
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
    if (!bestGood) return;
    // Add best good to offer if found.
    traveler.offerGood(*bestGood);
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
        if (!overWeight || carry < 0) {
            auto &gMs = gI->getMaterials();
            for (auto mI = gMs.begin(); mI != gMs.end(); ++mI) {
                auto &rM = gs[gI->getId()].getMaterial(*mI);
                score = rM.getBuyScore(mI->getPrice(),
                                       mII->buy); // score based on maximum buy price
                ++mII;
                if (score > bestScore) {
                    bestScore = score;
                    // Remove amout town takes as profit, store excess.
                    excess = 0;
                    amount = mI->getQuantity(offerValue * Settings::getTownProfit(), excess);
                    if (amount > 0.) {
                        if (overWeight) {
                            // Try to buy minimum that will bring net weight back below 0
                            double needed = ceil(weight / -carry);
                            if (amount > needed) {
                                excess += amount - needed;
                                amount = needed;
                            }
                        }
                        if (!gI->getSplit())
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
    if (!bestGood) return;
    if (excess > 0.) traveler.divideExcess(excess);
    traveler.requestGood(*bestGood);
    // Make the trade.
    traveler.makeTrade();
}

void AI::autoEquip() {
    // Equip best scoring item for each part.
    std::array<double, 6> bestScore;
    auto &gs = traveler.getGoods();
    for (auto &g : gs)
        if (g.getAmount() >= 1.)
            for (auto &m : g.getMaterials())
                if (m.getAmount() >= 1.) {
                    auto &ss = m.getCombatStats();
                    if (!ss.empty()) {
                        Good e(g.getId(), g.getName(), 1.);
                        Material eM(m.getId(), m.getName(), 1.);
                        eM.setCombatStats(ss);
                        e.addMaterial(eM);
                        double score = equipScore(e, traveler.getStats());
                        unsigned int pId = ss.front().partId;
                        if (score > bestScore[pId]) {
                            bestScore[pId] = score;
                            traveler.equip(e);
                        }
                    }
                }
}

void AI::autoAttack() {
    // Attack traveler with lower equipment score and lootable goods.
    if (!traveler.getTarget().lock()) {
        // There isn't already a target.
        auto able = traveler.attackable();
        double ourEquipScore = equipScore(traveler.getEquipment(), traveler.getStats());
        // Remove travelers who do not meet threshold for attacking.
        auto &gs = traveler.getGoods();
        able.erase(std::remove_if(able.begin(), able.end(),
                                  [this, ourEquipScore, &gs](const std::shared_ptr<Traveler> a) {
                                      return decisionCriteria[4] < Settings::getCriteriaMax() / 3 || 
                                             lootScore(a->getGoods()) * ourEquipScore * decisionCriteria[4] /
                                                     equipScore(a->getEquipment(), a->getStats()) <
                                                 Settings::getAIAttackThreshold();
                                  }),
                   able.end());
        if (!able.empty()) {
            // Attack the easiest target.
            auto easiest = std::min_element(
                able.begin(), able.end(), [this](const std::shared_ptr<Traveler> a, const std::shared_ptr<Traveler> b) {
                    return equipScore(a->getEquipment(), a->getStats()) < equipScore(b->getEquipment(), b->getStats());
                });
            traveler.attack(*easiest);
        }
    }
}

void AI::autoChoose() {
    // Choose to fight, run, or yield based on equip scores, stats, and speeds.
    std::array<double, 3> scores; // fight, run, yield scores
    auto target = traveler.getTarget().lock();
    double equipmentScoreRatio =
        equipScore(target->getGoods(), target->getStats()) / equipScore(traveler.getGoods(), traveler.getStats());
    scores[0] = 1. / equipmentScoreRatio * decisionCriteria[4];
    scores[1] = equipmentScoreRatio * decisionCriteria[5];
    scores[2] = equipmentScoreRatio * decisionCriteria[6];
    if (equipmentScoreRatio > 1.) {
        // Target's equipment is better, weigh run and yield decisions by speed
        // ratio.
        double speedRatio = target->getSpeed() / traveler.getSpeed();
        scores[1] /= speedRatio;
        scores[2] *= speedRatio;
    }
    traveler.choose(static_cast<FightChoice>(std::max_element(scores.begin(), scores.end()) - scores.begin()));
}

void AI::autoLoot() {
    // Automatically loot based on scores and decision criteria.
    auto &gs = traveler.getGoods();
    auto tgt = traveler.getTarget().lock();
    if (!tgt) return;
    auto &tGs = tgt->getGoods();
    double lootGoal = lootScore(tGs);
    if (tgt->alive())
        // Looting from an alive target dependent on greed.
        lootGoal *= decisionCriteria[7] / Settings::getCriteriaMax();
    double looted = 0.; // value that has already been looted
    double weight = traveler.netWeight();
    while (looted < lootGoal) {
        // Keep looting until amount looted matches goal or net weight is above 0.
        std::unique_ptr<Good> bestGood;
        double bestScore = 0;
        auto mII = materialInfo.begin(); // material info iterator
        for (size_t i = 0; i < gs.size(); ++i) {
            auto &gMs = gs[i].getMaterials(); // good materials
            auto &tG = tGs[i];                // target good
            auto &tGMs = tG.getMaterials();   // target good materials
            for (unsigned int j = 0; j < gMs.size(); ++j) {
                auto &tGM = tGMs[j]; // target good material
                double score = tGM.getAmount() * mII->value / tG.getCarry();
                if ((bestScore >= 0. && score > bestScore) || (score < 0. && score < bestScore)) {
                    bestScore = score;
                    bestGood = std::make_unique<Good>(i, tGM.getAmount());
                    bestGood->addMaterial(Material(j, tGM.getAmount()));
                }
                ++mII;
            }
        }
        if (!bestGood || weight + bestGood->getCarry() * bestGood->getAmount() > 0.) break;
        looted += bestScore * gs[bestGood->getId()].getCarry();
        // Loot the current best good from target.
        traveler.loot(*bestGood, *tgt);
        // Add the weight of looted good to weight variable.
        weight += bestGood->getCarry();
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
            if (n != tT &&
                std::find_if(nearby.begin(), nearby.end(), [n](TownInfo t) { return t.town == n; }) == nearby.end()) {
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
        n.buyScore = 0.;
        n.sellScore = 0.;
    }
    auto &gs = traveler.getGoods();
    size_t gC = gs.size();           // good count
    auto mII = materialInfo.begin(); // material info iterator
    // Find minimum and maximum price for each material of each good in nearby towns.
    for (size_t i = 0; i < gC; ++i) {
        // Loop through goods.
        size_t mC = gs[i].getMaterials().size(); // materials count
        for (size_t j = 0; j < mC; ++j) {
            // Loop through materials.
            auto &gM = gs[i].getMaterial(j);
            bool hasMaterial = gM.getAmount() > 0.; // Traveler has non-zero amount of material j
            std::vector<double> prices;             // prices for this material of this good in
                                                    // all nearby towns that sell it
            prices.reserve(nC);
            for (auto &n : nearby) {
                // Loop through nearby towns to collect price info.
                auto &nM = n.town->getGood(i).getMaterial(j);
                double price = nM.getPrice();
                if (price > 0.)
                    // Good i material j is sold in nearby town k.
                    prices.push_back(price);
            }
            if (!prices.empty()) {
                auto mMP = std::minmax_element(prices.begin(), prices.end());
                mII->minPrice = *mMP.first;
                mII->maxPrice = *mMP.second;
                mII->value = *mMP.first + mII->limitFactor * (*mMP.second - *mMP.first);
                mII->buy = mII->value * Settings::getTownProfit();
                mII->sell = mII->value / Settings::getTownProfit();
            }
            for (auto &n : nearby) {
                // Loop through nearby towns again now that material info has been
                // gathered
                auto &nM = n.town->getGood(i).getMaterial(j);
                double price = nM.getPrice();
                if (hasMaterial && price > 0.) {
                    double sellScore = gM.getSellScore(price, mII->sell);
                    if (sellScore > n.sellScore) n.sellScore = sellScore;
                } else {
                    // Currently only scores buying materials not already owned.
                    double buyScore = gM.getBuyScore(price, mII->buy);
                    if (buyScore > n.buyScore) n.buyScore = buyScore;
                }
            }
            ++mII;
        }
    }
}

void AI::run(unsigned int e) {
    // Run the AI for the elapsed time. Includes trading, equipping, and attacking.
    if (!traveler.getMoving()) {
        decisionCounter += e;
        if (decisionCounter > Settings::getAIDecisionTime()) {
            autoTrade();
            autoEquip();
            autoAttack();
            size_t nC = nearby.size();
            std::vector<double> townScores(nC); // scores for going to each town
            // Set scores based on buying and selling prices in each town.
            std::transform(nearby.begin(), nearby.end(), townScores.begin(), [this](const TownInfo &tI) {
                return tI.buyScore * decisionCriteria[0] + tI.sellScore * decisionCriteria[1];
            });
            auto bestTownScore = std::max_element(townScores.begin(), townScores.end());
            if (bestTownScore != townScores.end()) {
                traveler.pickTown(nearby[static_cast<size_t>(bestTownScore - townScores.begin())].town);
                const Town *toTown = traveler.getTown();
                setNearby(toTown, toTown, Settings::getAITownRange());
                setLimits();
            }
            decisionCounter -= Settings::getAIDecisionTime();
        }
    }
}

flatbuffers::Offset<Save::AI> AI::save(flatbuffers::FlatBufferBuilder &b) const {
    auto sDecisionCriteria = b.CreateVector(std::vector<double>(decisionCriteria.begin(), decisionCriteria.end()));
    auto sMaterialInfo =
        b.CreateVectorOfStructs<Save::MaterialInfo>(materialInfo.size(), [this](size_t i, Save::MaterialInfo *mI) {
            *mI = Save::MaterialInfo(materialInfo[i].limitFactor, materialInfo[i].minPrice, materialInfo[i].maxPrice,
                                     materialInfo[i].value, materialInfo[i].buy, materialInfo[i].sell);
        });
    return Save::CreateAI(b, static_cast<short>(decisionCounter), sDecisionCriteria, sMaterialInfo);
}
