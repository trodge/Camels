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

AI::AI(Traveler &tvl, const EnumArray<double, DecisionCriteria> &dcC, const GoodInfoContainer &gsI, AIRole rl)
    : traveler(tvl), decisionCounter(Settings::aIDecisionCounter()),
      businessCounter(Settings::aIBusinessCounter()), decisionCriteria(dcC), goodsInfo(gsI), role(rl) {
    auto town = traveler.town();
    auto &townProperty = town->getProperty();
    // Insert full ids of owned goods into goods info.
    traveler.createAIGoods(role);
    for (auto fId : traveler.property().fullIds()) goodsInfo.insert({fId, true, Settings::aILimitFactor()});
    if (gsI.empty())
        // Insert a randomly chosen set of full ids from town into goods info.
        for (auto fId : Settings::aIFullIds(townProperty.fullIds()))
            goodsInfo.insert({fId, false, Settings::aILimitFactor()});
    setNearby(town, town, Settings::getAITownRange());
    setLimits();
}

AI::AI(Traveler &tvl, const Save::AI *ldAI) : traveler(tvl), decisionCounter(ldAI->decisionCounter()) {
    // Load AI from flatbuffers.
    auto lDecisionCriteria = ldAI->decisionCriteria();
    std::transform(lDecisionCriteria->begin(), lDecisionCriteria->end(), begin(decisionCriteria),
                   [](auto ld) { return ld; });
    auto lGoodsInfo = ldAI->goodsInfo();
    for (auto gII = lGoodsInfo->begin(); gII != lGoodsInfo->end(); ++gII)
        goodsInfo.insert({(*gII)->fullId(), (*gII)->owned(), (*gII)->limitFactor(), (*gII)->minPrice(),
                          (*gII)->maxPrice(), (*gII)->estimate(), (*gII)->buy(), (*gII)->sell()});
}

flatbuffers::Offset<Save::AI> AI::save(flatbuffers::FlatBufferBuilder &b) const {
    auto svDecisionCriteria = b.CreateVector(std::vector<double>(begin(decisionCriteria), end(decisionCriteria)));
    std::vector<GoodInfo> vGoodsInfo(begin(goodsInfo), end(goodsInfo));
    auto svGoodsInfo =
        b.CreateVectorOfStructs<Save::GoodInfo>(goodsInfo.size(), [vGoodsInfo](size_t i, Save::GoodInfo *gI) {
            *gI = Save::GoodInfo(vGoodsInfo[i].fullId, vGoodsInfo[i].owned, vGoodsInfo[i].limitFactor,
                                 vGoodsInfo[i].minPrice, vGoodsInfo[i].maxPrice, vGoodsInfo[i].estimate,
                                 vGoodsInfo[i].buy, vGoodsInfo[i].sell);
        });
    return Save::CreateAI(b, static_cast<short>(decisionCounter), svDecisionCriteria, svGoodsInfo);
}

double AI::attackScore(const Good &eq, const EnumArray<unsigned int, Stat> &sts) const {
    // Scores given equipment with given stats based on its attack value.
    double score = 0;
    for (auto &cS : eq.getCombatStats()) {
        score += cS.attack * sts[cS.stat];
        score *= cS.speed * sts[cS.stat];
    }
    return score;
}

double AI::attackScore(const std::vector<Good> &eqpmt, const EnumArray<unsigned int, Stat> &sts) const {
    // Scores given equipment with given stats based on its attack value.
    double score = 1;
    for (auto &eq : eqpmt) score += attackScore(eq, sts);
    return score;
}

double AI::defenseScore(const Good &eq, const EnumArray<unsigned int, Stat> &sts) const {
    // Scores given equipment with given stats based on its defense value.
    double score = 0;
    for (auto &cS : eq.getCombatStats())
        for (auto &d : cS.defense) score += d * sts[cS.stat];
    return score;
}

double AI::defenseScore(const std::vector<Good> &eqpmt, const EnumArray<unsigned int, Stat> &sts) const {
    // Scores given equipment with given stats based on its defense value.
    double score = 1;
    for (auto &eq : eqpmt) score += defenseScore(eq, sts);
    return score;
}

double AI::equipScore(const Good &eq, const EnumArray<unsigned int, Stat> &sts) const {
    // Scores parameter equipment based on parameter stats and this ai's criteria. Score is always >= 0.
    return attackScore(eq, sts) * decisionCriteria[DecisionCriteria::attackScoreWeight] +
           defenseScore(eq, sts) * decisionCriteria[DecisionCriteria::defenseScoreWeight];
}

double AI::equipScore(const std::vector<Good> &eqpmt, const EnumArray<unsigned int, Stat> &sts) const {
    // Scores parameter equipment with parameter stats based on this ai's criteria. Score is always >= 1.
    double score = 1;
    for (auto &eq : eqpmt) score += equipScore(eq, sts);
    return score;
}

double AI::equipScore(const Good &eq, const std::vector<Good> &eqpmt, const EnumArray<unsigned int, Stat> &sts) const {
    // Score single equipment against set of equipment where parts match
    std::vector<Part> pts;
    auto &cSs = eq.getCombatStats();
    if (cSs.empty()) return 0;
    pts.reserve(cSs.size());
    std::transform(begin(cSs), end(cSs), std::back_inserter(pts), [](auto &cS) { return cS.part; });
    for (auto &oEq : eqpmt)
        for (auto &cS : oEq.getCombatStats())
            for (auto &pt : pts)
                if (cS.part == pt) return std::max(equipScore(eq, sts) - equipScore(oEq, sts), 0.);
    return equipScore(eq, sts);
}

double AI::lootScore(const Property &tgtPpt) {
    // Total value from looting given set of goods.
    double score = 0;
    tgtPpt.forGood([this, &score](const Good &tgtGd) {
        // Attempt to insert the good to goods info.
        auto gII = goodsInfo.insert({tgtGd.getFullId(), false, Settings::aILimitFactor()}).first;
        score += tgtGd.getAmount() * gII->estimate;
    });
    return score;
}

void AI::choosePlan(std::vector<BusinessPlan> &plns, BusinessPlan *&bstPln, double dcCt, double &hst) {
    for (auto &pln : plns) {
        // Reduce score by cost of build plan.
        double score = pln.profit - pln.cost;
        // Multiply score by decision criteria.
        score *= dcCt;
        if (score > hst) {
            // Score for this build plan is better than current best score.
            hst = score;
            bstPln = &pln;
        }
    }
}

void AI::trade() {
    // Attempt to make best possible single trade in current town.
    double criteriaMax = Settings::getAIDecisionCriteriaMax();
    auto town = traveler.town();
    auto townId = town->getId();
    auto storageProperty = traveler.property(townId);
    if (storageProperty)
        // Take all goods out of storage.
        storageProperty->forGood([this](const Good &gd) {
            Good wG(gd);
            traveler.withdraw(wG);
            goodsInfo.insert({gd.getFullId(), true, Settings::aILimitFactor()});
        });
    // Clear offer and request from previous trade.
    traveler.clearTrade();
    double highest = 0, offerValue = 0, offerWeight = 0, weight = traveler.weight();
    bool overWeight = weight > 0; // goods are too heavy to carry
    std::unique_ptr<Good> bestGood;
    // Find highest sell score.
    auto &travelerProperty = traveler.property();
    auto &byOwned = goodsInfo.get<Owned>();
    auto rng = byOwned.equal_range(true);
    decltype(rng.first) sellInfo; // info for good sold
    for (; rng.first != rng.second; ++rng.first) {
        auto &gd = *travelerProperty.good(rng.first->fullId);
        double gWgt = gd.weight();
        if (!overWeight || gWgt > 0) {
            // Either we are not over weight or given material doesn't help carry.
            auto fId = gd.getFullId();
            auto tnGd = traveler.town()->getProperty().good(fId);
            if (tnGd == nullptr) return;
            auto amount = gd.getAmount();
            if (amount > 0) {
                if (gWgt < 0 && weight > gWgt)
                    // This good is needed to carry existing goods, reduce amount.
                    amount *= weight / gWgt;
                double score = sellScore(tnGd->price(), rng.first->sell); // score based on minimum sell price
                if ((overWeight && (!bestGood || gWgt > bestGood->weight())) || (score > highest)) {
                    // Either we are over weight and good is heavier than previous offer or this good scores better.
                    highest = score;
                    if (!gd.getSplit()) amount = floor(amount);
                    offerValue = tnGd->price(amount);
                    offerWeight = gWgt;
                    bestGood = std::make_unique<Good>(tnGd->getFullId(), tnGd->getFullName(), amount, tnGd->getMeasure());
                    sellInfo = rng.first;
                }
            }
        }
    }
    // Reduce offer value to reflect town profit.
    double townProfit = Settings::getTownProfit();
    offerValue *= townProfit;
    weight -= offerWeight;
    overWeight = weight > 0;
    if (overWeight)
        // Force a trade to occur.
        highest = 0;
    else
        // Trade only if score exceeds reciprocol of selling score.
        highest = 1 / highest;
    // Determine which businesses can be built based on offer value and town prices.
    auto &townProperty = town->getProperty();
    std::vector<BusinessPlan> buildPlans, // plans to build businesses in current town
        restockPlans;                     // plans to restock businesses in current town
    BusinessPlan *bestPlan = nullptr;     // pointer to highest scoring business plan
    if (role == AIRole::trader) {
        if (++businessCounter >= 0 && (!home || town == home)) {
            // Find best business scored based on requirements, inputs, and outputs.
            buildPlans = townProperty.buildPlans(travelerProperty, offerValue);
            choosePlan(buildPlans, bestPlan, decisionCriteria[DecisionCriteria::buildTendency] / criteriaMax, highest);
            if (bestPlan && bestPlan->cost == 0) {
                // Build business without trading.
                traveler.build(bestPlan->business, bestPlan->factor);
                if (!storageProperty) storageProperty = traveler.property(townId);
                store(storageProperty, travelerProperty);
                bestPlan = nullptr;
            }
            if (storageProperty) {
                restockPlans = townProperty.restockPlans(travelerProperty, *storageProperty, offerValue);
                choosePlan(restockPlans, bestPlan,
                           decisionCriteria[DecisionCriteria::restockTendency] / criteriaMax, highest);
            }
            businessCounter = -Settings::getAIBusinessInterval();
        }
    }
    if (!bestGood) return;
    // Add best selling good to offer if found.
    traveler.offerGood(std::move(*bestGood));
    bestGood = nullptr;
    double excess;
    // Find highest buy score among goods not owned.
    auto &equipment = traveler.getEquipment();
    auto &stats = traveler.getStats();
    rng = byOwned.equal_range(false);
    decltype(rng.first) buyInfo; // info for good bought
    for (; rng.first != rng.second; ++rng.first) {
        auto tnGd = townProperty.good(rng.first->fullId);
        if (!tnGd) return;
        double carry = tnGd->getCarry();
        if (!overWeight || carry < 0) {
            auto fId = tnGd->getFullId();
            double score = buyScore(tnGd->price(), rng.first->buy); // score based on maximum buy price
            // Weigh equip score double if not a trader or agent.
            double eqpScr = equipScore(*tnGd, equipment, stats) *
                            (1 + !(role == AIRole::trader || role == AIRole::agent)) *
                            decisionCriteria[DecisionCriteria::equipScoreWeight] / criteriaMax;
            score += eqpScr;
            if (score > highest) {
                highest = score;
                // Remove amout town takes as profit, store excess.
                excess = 0;
                double amount = tnGd->quantity(offerValue * townProfit, excess);
                if (amount > 0) {
                    if (overWeight) {
                        // Try to buy minimum that will bring net weight back below 0
                        double needed = ceil(weight / -carry);
                        if (amount > needed) {
                            excess += amount - needed;
                            amount = needed;
                        }
                    }
                    if (!tnGd->getSplit())
                        // Remove extra portion of goods that don't split.
                        excess += modf(amount, &amount);
                    // Convert the excess from units of bought good to deniers.
                    excess = tnGd->price(excess);
                    bestGood = std::make_unique<Good>(tnGd->getFullId(), tnGd->getFullName(), amount, tnGd->getMeasure());
                    buyInfo = rng.first;
                }
            }
        }
    }
    auto toggleOwned = [](GoodInfo &gdInf) { gdInf.owned = !gdInf.owned; };
    if (bestGood) {
        // Purchasing a good exceeded score of building a business.
        if (excess > 0) traveler.divideExcess(excess, townProfit);
        traveler.requestGood(std::move(*bestGood));
        traveler.makeTrade();
        byOwned.modify(sellInfo, toggleOwned);
        byOwned.modify(buyInfo, toggleOwned);
    } else if (bestPlan) {
        // No good exceeded score of building business.
        excess = offerValue - bestPlan->cost;
        if (excess > 0) traveler.divideExcess(excess, townProfit);
        traveler.requestGoods(std::move(bestPlan->request));
        traveler.makeTrade();
        byOwned.modify(sellInfo, toggleOwned);
        if (bestPlan->build) {
            home = town;
            traveler.build(bestPlan->business, bestPlan->factor);
        }
        if (!storageProperty) storageProperty = traveler.property(townId);
        store(storageProperty, travelerProperty);
    }
}

void AI::store(const Property *sPpt, const Property &tnPpt) {
    // Deposit all goods that are inputs for businesses.
    for (auto &bsn : sPpt->getBusinesses())
        for (auto &ip : bsn.getInputs())
            tnPpt.forGood(ip.getGoodId(), [this](const Good &gd) {
                Good dG(gd);
                traveler.deposit(dG);
            });
}

void AI::equip() {
    // Equip best scoring item for each part.
    EnumArray<double, Part> bestScores;
    traveler.property().forGood([this, &bestScores](const Good &gd) {
        if (gd.getAmount() >= 1) {
            auto &ss = gd.getCombatStats();
            if (!ss.empty()) {
                Good e(gd.getFullId(), gd.getFullName(), 1, ss, gd.getImage());
                double score = equipScore(e, traveler.getStats());
                Part pt = ss.front().part;
                if (score > bestScores[pt]) {
                    bestScores[pt] = score;
                    traveler.equip(e);
                }
            }
        }
    });
}

void AI::attack() {
    // Attack traveler with lower equipment score and lootable goods.
    switch (role) {
    case AIRole::bandit:
        if (!traveler.getTarget()) {
            // There isn't already a target.
            auto able = traveler.attackable();
            double ourEquipScore = equipScore(traveler.getEquipment(), traveler.getStats());
            // Remove travelers who do not meet threshold for attacking.
            able.erase(std::remove_if(begin(able), end(able),
                                      [this, ourEquipScore](const Traveler *a) {
                                          return lootScore(a->property()) * ourEquipScore *
                                                     decisionCriteria[DecisionCriteria::fightTendency] /
                                                     equipScore(a->getEquipment(), a->getStats()) <
                                                 Settings::getAIAttackThreshold();
                                      }),
                       end(able));
            if (!able.empty()) {
                // Attack the easiest target.
                Traveler *easiest = nullptr;
                double lowest = std::numeric_limits<double>::max();
                for (auto tvl : able) {
                    double score = equipScore(tvl->getEquipment(), tvl->getStats());
                    if (score < lowest) {
                        lowest = score;
                        easiest = tvl;
                    }
                }
                traveler.attack(easiest);
            }
        }
        break;
    default:
        break;
    }
}

FightChoice AI::choice() {
    // Choose to fight, update, or yield based on equip scores, stats, and speeds.
    EnumArray<double, FightChoice> scores; // fight, run, yield scores
    auto target = traveler.getTarget();
    double equipmentScoreRatio = equipScore(target->getEquipment(), target->getStats()) /
                                 equipScore(traveler.getEquipment(), traveler.getStats());
    scores[FightChoice::fight] = 1 / equipmentScoreRatio * decisionCriteria[DecisionCriteria::fightTendency];
    scores[FightChoice::run] = equipmentScoreRatio * decisionCriteria[DecisionCriteria::runTendency];
    scores[FightChoice::yield] = equipmentScoreRatio * decisionCriteria[DecisionCriteria::yieldTendency];
    if (equipmentScoreRatio > 1)
        // Target's equipment is better, weigh run score by speed ratio.
        scores[FightChoice::run] *= traveler.speed() / target->speed();
    // Choose whichever choice has the best score.
    return static_cast<FightChoice>(std::max_element(begin(scores), end(scores)) - begin(scores));
}

Traveler *AI::target(const std::unordered_set<Traveler *> &enms) const {
    Traveler *tgt = nullptr;
    double highest = 0;
    for (auto enm : enms) {
        enm->forAlly([this, &tgt, &highest](Traveler *aly) {
            if (!aly->alive()) return;
            auto &eqpmt = aly->getEquipment();
            auto &sts = aly->getStats();
            unsigned int tgtrCnt = aly->getTargeterCount();
            double score = attackScore(eqpmt, sts) / defenseScore(eqpmt, sts) / tgtrCnt;
            if (score > highest) {
                highest = score;
                tgt = aly;
            }
        });
    }
    return tgt;
}

Traveler *AI::lootTarget(const std::unordered_set<Traveler *> &enms) {
    Traveler *tgt = nullptr;
    double highest = 0;
    for (auto enm : enms) {
        enm->forAlly([this, &highest, &tgt](Traveler *aly) {
            double score = lootScore(aly->property());
            if (score > highest) {
                highest = score;
                tgt = aly;
            }
        });
    }
    return tgt;
}

void AI::loot() {
    // Automatically loot based on scores and decision criteria.
    auto enemies = traveler.getEnemies();
    double lootGoal = std::accumulate(begin(enemies), end(enemies), 0., [this](double a, const Traveler *enm) {
        return a + lootScore(enm->property());
    });
    auto target = lootTarget(enemies);
    auto targetProperty = &target->property();
    if (target->alive())
        // Looting from an alive target dependent on greed.
        lootGoal *= decisionCriteria[DecisionCriteria::lootingGreed] / Settings::getAIDecisionCriteriaMax();
    double looted = 0, weight = traveler.weight();
    while (looted < lootGoal) {
        // Keep looting until amount looted matches goal or we can carry no more.
        double highest = 0, bestValue, bestWeight;
        std::unique_ptr<Good> bestGood;
        GoodInfoContainer::iterator bestGoodInfo;
        targetProperty->forGood([this, &highest, &bestValue, &bestWeight, &bestGood, &bestGoodInfo, looted,
                         lootGoal](const Good &tgtGd) {
            double amount = tgtGd.getAmount();
            if (amount > 0) {
                auto gII = goodsInfo.insert({tgtGd.getFullId(), false, Settings::aILimitFactor()}).first;
                double estimate = gII->estimate;
                double carry = tgtGd.getCarry();
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
                if ((highest >= 0 && score > highest) || (score < 0 && score < highest)) {
                    // Either this is more valuable per weight than current best or this helps carry more than current best.
                    highest = score;
                    amount = std::min((lootGoal - looted) / estimate, amount);
                    bestValue = estimate * amount;
                    bestWeight = carry * amount;
                    bestGood = std::make_unique<Good>(tgtGd.getFullId(), amount);
                    bestGoodInfo = gII;
                }
            }
        });
        if (!bestGood) {
            // Target has no more goods to loot.
            enemies.erase(target);
            target = lootTarget(enemies);
            targetProperty = &target->property();
            continue;
        }
        // Add the weight of looted good to weight variable.
        weight += bestWeight;
        // Stop looting if we would be overweight.
        if (weight > 0) return;
        // Loot the current best good from target.
        traveler.loot(*bestGood);
        goodsInfo.modify(bestGoodInfo, [](GoodInfo &gdInf) { gdInf.owned = true; });
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
    for (auto &nb : nearby) {
        // Reset buy/sell scores
        nb.buyScore = 0;
        nb.sellScore = 0;
        // Find minimum and maximum price for each good in nearby towns.
        auto &townProperty = nb.town->getProperty();
        for (auto gdInfIt = begin(goodsInfo); gdInfIt != end(goodsInfo); ++gdInfIt) {
            auto nbGd = townProperty.good(gdInfIt->fullId);
            if (!nbGd) continue;
            auto price = nbGd->price();
            goodsInfo.modify(gdInfIt, [price](GoodInfo &gdInf) {
                gdInf.minPrice = std::min(price, gdInf.minPrice);
                gdInf.maxPrice = std::max(price, gdInf.maxPrice);
            });
        }
    }
    // Set value and buy and sell based on min and max prices.
    double townProfit = Settings::getTownProfit();
    for (auto gdInfIt = begin(goodsInfo); gdInfIt != end(goodsInfo); ++gdInfIt) {
        goodsInfo.modify(gdInfIt, [townProfit](GoodInfo &gdInf) {
            gdInf.estimate = gdInf.minPrice + gdInf.limitFactor * (gdInf.maxPrice - gdInf.minPrice);
            gdInf.buy = gdInf.estimate * townProfit;
            gdInf.sell = gdInf.estimate / townProfit;
        });
    }
    auto &ppt = traveler.property();
    // Loop through nearby towns again now that info has been gathered to set buy and sell scores.
    for (auto &nb : nearby) {
        auto &townProperty = nb.town->getProperty();
        auto &byOwned = goodsInfo.get<Owned>();
        // Set sell scores for goods owned.
        auto rng = byOwned.equal_range(true);
        std::for_each(rng.first, rng.second, [&nb, &townProperty](const GoodInfo &gdInf) {
            auto nbGd = townProperty.good(gdInf.fullId);
            if (!nbGd) return;
            nb.sellScore = std::max(sellScore(nbGd->price(), gdInf.sell), nb.sellScore);
        });
        // Set buy scores for goods not owned.
        rng = byOwned.equal_range(false);
        std::for_each(rng.first, rng.second, [&nb, &townProperty](const GoodInfo &gdInf) {
            auto nbGd = townProperty.good(gdInf.fullId);
            if (!nbGd) return;
            nb.buyScore = std::max(buyScore(nbGd->price(), gdInf.buy), nb.buyScore);
        });
    }
}

void AI::update(unsigned int elTm) {
    // Run the AI for the elapsed time. Includes trading, equipping, and attacking.
    if (!traveler.getMoving()) {
        decisionCounter += elTm;
        if (decisionCounter > 0) {
            trade();
            equip();
            attack();
            // Find highest score based on buy and sell scores in each town.
            const Town *bestTown = nullptr;
            if (businessCounter >= 0 && home)
                // Return to home.
                bestTown = home;
            else {
                // Find best scoring
                double highest = 0;
                for (auto &tI : nearby) {
                    double score = tI.buyScore * decisionCriteria[DecisionCriteria::buyScoreWeight] +
                                   tI.sellScore * decisionCriteria[DecisionCriteria::sellScoreWeight];
                    if (score > highest) {
                        highest = score;
                        bestTown = tI.town;
                    }
                }
            }
            if (bestTown) {
                // Set traveler's target to target town.
                traveler.pickTown(bestTown);
                // Set nearby towns based on town traveler travels to first.
                const Town *town = traveler.town();
                setNearby(town, town, Settings::getAITownRange());
                setLimits();
            }
            decisionCounter -= Settings::getAIDecisionTime();
        }
    }
}
