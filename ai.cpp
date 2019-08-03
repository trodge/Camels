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
    auto &tWs = Settings::getAITypeWeights();
    std::discrete_distribution<> dis(begin(tWs), end(tWs));
    type = static_cast<Type>(dis(rng));
    randomizeLimitFactors();
    randomizeCriteria();
    const Town *toTown = traveler.toTown;
    setNearby(toTown, toTown, Settings::getAITownRange());
    setLimits();
}

AI::AI(Traveler &tvl, const AI &p) : traveler(tvl), decisionCriteria(p.decisionCriteria), materialInfo(p.materialInfo) {
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
        materialInfo.push_back({(*mII)->limitFactor(), (*mII)->minPrice(), (*mII)->maxPrice(), (*mII)->value(),
                                (*mII)->buy(), (*mII)->sell()});
}

flatbuffers::Offset<Save::AI> AI::save(flatbuffers::FlatBufferBuilder &b) const {
    auto sDecisionCriteria = b.CreateVector(std::vector<double>(begin(decisionCriteria), end(decisionCriteria)));
    auto sMaterialInfo =
        b.CreateVectorOfStructs<Save::MaterialInfo>(materialInfo.size(), [this](size_t i, Save::MaterialInfo *mI) {
            *mI = Save::MaterialInfo(materialInfo[i].limitFactor, materialInfo[i].minPrice, materialInfo[i].maxPrice,
                                     materialInfo[i].value, materialInfo[i].buy, materialInfo[i].sell);
        });
    return Save::CreateAI(b, static_cast<short>(decisionCounter), sDecisionCriteria, sMaterialInfo);
}

void AI::randomizeLimitFactors() {
    // Randomize factors which will be used to choose buy and sell limits for each material.
    auto &gs = traveler.goods;
    // Total count of materials across all goods, including labor.
    size_t mC =
        std::accumulate(begin(gs), end(gs), 0, [](unsigned int c, const Good &g) { return c + g.getMaterials().size(); });
    materialInfo = std::vector<MaterialInfo>(mC);
    static std::uniform_real_distribution<double> dis(Settings::getLimitFactorMin(), Settings::getLimitFactorMax());
    auto mII = begin(materialInfo);
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
    static std::uniform_int_distribution<> iDis(-Settings::getAIDecisionTime(), 0);
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
    // Total value from looting given set of goods.
    double score = 0.;
    auto mII = begin(materialInfo); // material info iterator
    for (auto &tG : tGs)
        for (auto &tGM : tG.getMaterials()) score += tGM.getAmount() * mII++->value;
    return score;
}

void AI::trade() {
    // Attempt to make best possible single trade in current town.
    traveler.clearTrade();
    double amount, score, offerValue = 0, weight = traveler.weight(), offerWeight = 0., bestScore = 0.;
    bool overWeight = weight > 0.;
    std::unique_ptr<Good> bestGood;
    // Find highest sell score.
    auto mII = begin(materialInfo); // material info iterator
    auto &tGs = traveler.toTown->getGoods();
    // Loop through goods excluding labor.
    for (auto &g : traveler.goods)
        for (auto &m : g.getMaterials()) {
            double mWgt = m.weight();
            if (!overWeight || mWgt > 0.) {
                // Either we are not over weight or given material doesn't help carry.
                auto &tM = tGs[g.getId()].getMaterial(m);
                amount = m.getAmount();
                if (amount > 0.) {
                    if (mWgt < 0. && weight > mWgt)
                        // This good is needed to carry existing goods, reduce amount.
                        amount *= weight / mWgt;
                    if (amount <= 0) continue;
                    score = m.sellScore(tM.price(), mII->sell); // score based on minimum sell price
                    if ((overWeight && mWgt > offerWeight) || (score > bestScore)) {
                        // Either we are over weight and good is heavier than previous offer or this good scores better.
                        bestScore = score;
                        if (!g.getSplit()) amount = floor(amount);
                        bestGood = std::make_unique<Good>(g.getId(), g.getName(), amount, g.getMeasure());
                        bestGood->addMaterial(Material(m.getId(), m.getName(), amount));
                        offerValue = tM.price(amount);
                        offerWeight = mWgt;
                    }
                }
            }
            ++mII;
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
    mII = begin(materialInfo); // material info iterator
    // Loop through goods.
    for (auto &tG : tGs)
        for (auto &tM : tG.getMaterials()) {
            double carry = tM.getCarry();
            if (!overWeight || carry < 0.) {
                auto &rM = traveler.goods[tG.getId()].getMaterial(tM);
                score = rM.buyScore(tM.price(),
                                    mII->buy); // score based on maximum buy price
                ++mII;
                if (score > bestScore) {
                    bestScore = score;
                    // Remove amout town takes as profit, store excess.
                    excess = 0.;
                    amount = tM.quantity(offerValue * townProfit, excess);
                    if (amount > 0.) {
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
                        excess = tM.price(excess);
                        bestGood = std::make_unique<Good>(tG.getId(), tG.getName(), amount, tG.getMeasure());
                        bestGood->addMaterial(Material(tM.getId(), tM.getName(), amount));
                    }
                }
            }
        }
    if (!bestGood) return;
    if (excess > 0.) traveler.divideExcess(excess);
    traveler.request.push_back(*bestGood);
    // Make the trade.
    traveler.makeTrade();
}

void AI::equip() {
    // Equip best scoring item for each part.
    std::array<double, 6> bestScore;
    auto &gs = traveler.goods;
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
        auto &gs = traveler.goods;
        able.erase(std::remove_if(begin(able), end(able),
                                  [this, ourEquipScore, &gs](const Traveler *a) {
                                      return decisionCriteria[4] < Settings::getCriteriaMax() / 3 ||
                                             lootScore(a->getGoods()) * ourEquipScore * decisionCriteria[4] /
                                                     equipScore(a->getEquipment(), a->getStats()) <
                                                 Settings::getAIAttackThreshold();
                                  }),
                   end(able));
        if (!able.empty()) {
            // Attack the easiest target.
            auto easiest = std::min_element(begin(able), end(able), [this](const Traveler *a, const Traveler *b) {
                return equipScore(a->getEquipment(), a->getStats()) < equipScore(b->getEquipment(), b->getStats());
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
        equipScore(target->getGoods(), target->getStats()) / equipScore(traveler.goods, traveler.stats);
    scores[0] = 1. / equipmentScoreRatio * decisionCriteria[4];
    scores[1] = equipmentScoreRatio * decisionCriteria[5];
    scores[2] = equipmentScoreRatio * decisionCriteria[6];
    if (equipmentScoreRatio > 1.) {
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
    auto &tGs = tgt->goods;
    double lootGoal = lootScore(tGs);
    if (tgt->alive())
        // Looting from an alive target dependent on greed.
        lootGoal *= decisionCriteria[7] / Settings::getCriteriaMax();
    double looted = 0.; // value that has already been looted
    double weight = traveler.weight();
    while (looted < lootGoal) {
        // Keep looting until amount looted matches goal or we can carry no more.
        std::unique_ptr<Good> bestGood;
        double bestScore = 0.;
        double bestLooted = 0.;
        double bestWeight = 0.;
        auto mII = begin(materialInfo); // material info iterator
        for (auto &tG : tGs) {
            for (auto &tGM : tG.getMaterials()) {
                double amount = tGM.getAmount();
                if (amount > 0.) {
                    double value = mII->value;
                    double carry = tGM.getCarry();
                    double score;
                    if (carry > 0.)
                        // Heavier goods score lower.
                        score = value / carry;
                    else if (carry == 0.)
                        // Weightless goods score highest.
                        score = std::numeric_limits<double>::max();
                    else
                        // Goods that carry other good score negative.
                        score = carry;
                    if ((bestScore >= 0. && score > bestScore) || (score < 0. && score < bestScore)) {
                        // Either this is more valuable per weight than current best or this helps carry more than current best.
                        bestScore = score;
                        if (looted + amount * value > lootGoal) amount = (lootGoal - looted) / value;
                        bestLooted = amount * value;
                        bestGood = std::make_unique<Good>(tG.getId(), amount);
                        bestGood->addMaterial(Material(tGM.getId(), amount));
                        bestWeight = amount * carry;
                    }
                }
                ++mII;
            }
        }
        if (!bestGood || weight + bestGood->getMaterial().weight() > 0.) break;
        looted += bestLooted;
        // Loot the current best good from target.
        traveler.loot(*bestGood, *tgt);
        // Add the weight of looted good to weight variable.
        weight += bestWeight;
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
        n.buyScore = 0.;
        n.sellScore = 0.;
    }
    auto &gs = traveler.goods;
    size_t gC = gs.size();          // good count
    auto mII = begin(materialInfo); // material info iterator
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
                double price = nM.price();
                if (price > 0.)
                    // Good i material j is sold in nearby town k.
                    prices.push_back(price);
            }
            if (!prices.empty()) {
                auto mMP = std::minmax_element(begin(prices), end(prices));
                mII->minPrice = *mMP.first;
                mII->maxPrice = *mMP.second;
                mII->value = *mMP.first + mII->limitFactor * (*mMP.second - *mMP.first);
                mII->buy = mII->value * Settings::getTownProfit();
                mII->sell = mII->value / Settings::getTownProfit();
            }
            for (auto &n : nearby) {
                // Loop through nearby towns again now that material info has been gathered.
                auto &nM = n.town->getGood(i).getMaterial(j);
                double price = nM.price();
                if (hasMaterial && price > 0.) {
                    double sellScore = gM.sellScore(price, mII->sell);
                    if (sellScore > n.sellScore) n.sellScore = sellScore;
                } else {
                    // Currently only scores buying materials not already owned.
                    double buyScore = gM.buyScore(price, mII->buy);
                    if (buyScore > n.buyScore) n.buyScore = buyScore;
                }
            }
            ++mII;
        }
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
