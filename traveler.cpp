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

#include "traveler.h"

Traveler::Traveler(const std::string &n, Town *t, const GameData *gD)
    : name(n), toTown(t), fromTown(t), nation(t->getNation()), longitude(t->getLongitude()), latitude(t->getLatitude()),
      gameData(gD), moving(false) {
    // Copy goods vector from nation.
    const std::vector<Good> &gs = t->getNation()->getGoods();
    goods.reserve(gs.size());
    for (auto &g : gs) {
        goods.push_back(Good(g));
    }
    // Create starting goods.
    goods[55].create(0.75);
    goods[59].create(2);
    // Equip fists.
    equip(2);
    equip(3);
    // Randomize stats.
    static std::uniform_int_distribution<> dis(1, Settings::getStatMax());
    for (auto &s : stats)
        s = dis(*Settings::getRng());
    // Set parts status to normal.
    parts.fill(0);
}

Traveler::Traveler(const Save::Traveler *t, std::vector<Town> &ts, const std::vector<Nation> &ns, const GameData *gD)
    : name(t->name()->str()), toTown(&ts[t->town() - 1]), fromTown(&ts[t->oldTown() - 1]), nation(&ns[t->nation() - 1]),
      longitude(t->longitude()), latitude(t->latitude()), gameData(gD), moving(t->moving()) {
    auto lLog = t->log();
    for (auto lLI = lLog->begin(); lLI != lLog->end(); ++lLI)
        logText.push_back(lLI->str());
    auto lGoods = t->goods();
    for (auto lGI = lGoods->begin(); lGI != lGoods->end(); ++lGI)
        goods.push_back(Good(*lGI));
    auto lStats = t->stats();
    for (size_t i = 0; i < stats.size(); ++i)
        stats[i] = lStats->Get(i);
    auto lParts = t->parts();
    for (size_t i = 0; i < parts.size(); ++i)
        parts[i] = lParts->Get(i);
    auto lEquipment = t->equipment();
    for (auto lEI = lEquipment->begin(); lEI != lEquipment->end(); ++lEI)
        equipment.push_back(Good(*lEI));
}

void Traveler::addToTown() { fromTown->addTraveler(shared_from_this()); }

double Traveler::netWeight() const {
    return std::accumulate(goods.begin(), goods.end(), stats[0] * -16,
                           [](double d, Good g) { return d + g.getCarry() * g.getAmount(); });
}

std::forward_list<Town *> Traveler::pathTo(const Town *t) const {
    // return forward list of towns on shortest path to t, excluding current town
    std::forward_list<Town *> path;
    // the town which each town can be most efficiently reached from
    std::unordered_map<Town *, Town *> from;
    // distance along route to each town
    std::unordered_map<Town *, double> distTo({{toTown, 0}});
    // estimated distance along route through each town to goal
    std::unordered_map<Town *, double> distEst({{toTown, toTown->dist(t)}});
    // set of towns already evaluated
    std::unordered_set<Town *> closed;
    auto shortest = [&distEst](Town *m, Town *n) {
        double c = distEst[m];
        double d = distEst[n];
        if (c == d)
            return m->getId() < n->getId();
        return c < d;
    };
    // set of discovered towns not yet evaluated sorted so that shortest is first
    std::set<Town *, decltype(shortest)> open(shortest);
    open.insert(toTown);
    while (not open.empty()) {
        // find current town with lowest distance
        Town *current = *open.begin();
        const std::vector<Town *> &neighbors = current->getNeighbors();
        if (current == t) {
            while (from.count(current)) {
                path.push_front(current);
                current = from[current];
            }
            return path;
        }
        open.erase(open.begin());
        closed.insert(current);
        for (auto &n : neighbors) {
            // ignore town if already explored
            if (closed.count(n))
                continue;
            // find distance to n through current
            double dT = distTo[current] + current->dist(n);
            // check if distance through current is less than previous shortest path to n
            if (not distTo.count(n) or dT < distTo[n]) {
                from[n] = current;
                distTo[n] = dT;
                distEst[n] = dT + n->dist(t);
            }
            // add undiscovered node to open set
            if (not open.count(n))
                open.insert(n);
        }
    }
    return path;
}

double Traveler::distSq(int x, int y) const { return (px - x) * (px - x) + (py - y) * (py - y); }

double Traveler::pathDist(const Town *t) const {
    // return the distance to t along shortest path
    const std::forward_list<Town *> &path = pathTo(t);
    double dist = 0;
    const Town *m = toTown;
    for (auto &n : path) {
        dist += m->dist(n);
        m = n;
    }
    return dist;
}

void Traveler::changeTradePortion(double d) {
    tradePortion += d;
    if (tradePortion < 0)
        tradePortion = 0;
    else if (tradePortion > 1)
        tradePortion = 1;
}

void Traveler::pickTown(const Town *t) {
    if (netWeight() > 0 /* or moving*/)
        return;
    const std::forward_list<Town *> &path = pathTo(t);
    if (path.empty() or path.front() == toTown)
        return;
    moving = true;
    toTown = path.front();
}

void Traveler::place(int ox, int oy, double s) {
    px = int(s * longitude) + ox;
    py = oy - int(s * latitude);
}

void Traveler::draw(SDL_Surface *s) const {
    SDL_Color col;
    if (ai)
        col = Settings::getAIColor();
    else
        col = Settings::getPlayerColor();
    draw_circle(s, px, py, 1, col, true);
}

void Traveler::updateTradeButtons(std::vector<std::unique_ptr<TextBox>> &bs) {
    // Update the values shown on offer and request boxes and set offer and request.
    std::string bN;
    offer.clear();
    request.clear();
    double offerValue = 0;
    auto rBB = bs.begin() + requestButtonIndex; // iterator to first request button
    // Loop through all boxes after cancel button and before first request button.
    for (auto oBI = bs.begin() + 2; oBI != rBB; ++oBI) {
        auto &g = goods[(*oBI)->getId()]; // good corresponding to oBI
        auto &gMs = g.getMaterials();
        bN = (*oBI)->getText()[0];
        auto mI = std::find_if(gMs.begin(), gMs.end(), [bN](const Material &m) {
            const std::string &mN = m.getName();
            return mN.substr(0, mN.find(' ')) == bN.substr(0, bN.find(' '));
        });
        if (mI != gMs.end()) {
            // mI is iterator to the material on oBI
            mI->updateButton(g.getSplit(), 0, 0, oBI->get());
            if ((*oBI)->getClicked()) {
                double amount = mI->getAmount() * tradePortion;
                if (not g.getSplit())
                    amount = floor(amount);
                offer.push_back(Good(g.getId(), g.getName(), amount, g.getMeasure()));
                offer.back().addMaterial(Material(mI->getId(), mI->getName(), amount));
                auto &tM = toTown->getGood(g.getId()).getMaterial(*mI);
                offerValue += tM.getPrice(amount);
            }
        }
    }
    int requestCount =
        std::accumulate(rBB, bs.end(), 0, [](int c, const std::unique_ptr<TextBox> &rB) { return c + rB->getClicked(); });
    double excess = 0; // excess value of offer over value needed for request
    // Loop through request buttons.
    for (auto rBI = rBB; rBI != bs.end(); ++rBI) {
        auto &g = toTown->getGood((*rBI)->getId()); // good in town corresponding to rBI
        auto &gMs = g.getMaterials();
        bN = (*rBI)->getText()[0];
        auto mI = std::find_if(gMs.begin(), gMs.end(), [bN](Material m) {
            const std::string &mN = m.getName();
            return mN.substr(0, mN.find(' ')) == bN.substr(0, bN.find(' '));
        });
        if (mI != gMs.end()) {
            // mI is iterator to the material on rBI
            mI->updateButton(g.getSplit(), offerValue, requestCount ? requestCount : 1, rBI->get());
            if ((*rBI)->getClicked()) {
                double mE = 0; // excess quantity of this material
                double amount = mI->getQuantity(offerValue / requestCount * Settings::getTownProfit(), &mE);
                if (not g.getSplit()) {
                    // Remove extra portion of goods that don't split.
                    mE += modf(amount, &amount);
                }
                // Convert material excess to value and add to overall excess.
                excess += mI->getPrice(mE);
                request.push_back(Good(g.getId(), g.getName(), amount, g.getMeasure()));
                request.back().addMaterial(Material(mI->getId(), mI->getName(), amount));
            }
        }
    }
    // divide excess among offered goods
    excess /= offer.size();
    for (auto &of : offer) {
        auto &tM = toTown->getGood(of.getId()).getMaterial(of.getMaterial());
        double q = tM.getQuantity(excess / Settings::getTownProfit());
        if (not goods[of.getId()].getSplit())
            q = floor(q);
        of.use(q);
    }
}

void Traveler::makeTrade() {
    if (not(offer.empty() or request.empty())) {
        std::string logEntry = name + " trades ";
        for (auto gI = offer.begin(); gI != offer.end(); ++gI) {
            if (gI != offer.begin()) {
                // This is not the first good in offer.
                if (offer.size() != 2)
                    logEntry += ", ";
                if (gI + 1 == offer.end())
                    // This is the last good in offer.
                    logEntry += " and ";
            }
            goods[gI->getId()].take(*gI);
            toTown->put(*gI);
            logEntry += gI->logEntry();
        }
        logEntry += " for ";
        for (auto gI = request.begin(); gI != request.end(); ++gI) {
            if (gI != request.begin()) {
                // This is not the first good in request.
                if (request.size() != 2)
                    logEntry += ", ";
                if (gI + 1 == request.end())
                    // This is the last good in request.
                    logEntry += " and ";
            }
            toTown->take(*gI);
            goods[gI->getId()].put(*gI);
            logEntry += gI->logEntry();
        }
        logEntry += " in " + toTown->getName() + ".";
        logText.push_back(logEntry);
    }
}

void Traveler::unequip(int pI) {
    // Unequip all equipment using the given part id.
    auto unused = [pI](const Good &e) {
        auto &eSs = e.getMaterial().getCombatStats();
        return std::find_if(eSs.begin(), eSs.end(), [pI](const CombatStat &s) { return s.partId == pI; }) == eSs.end();
    };
    auto uEI = std::partition(equipment.begin(), equipment.end(), unused);
    // Put equipment back in goods.
    for (auto eI = uEI; eI != equipment.end(); ++eI)
        if (eI->getAmount())
            goods[eI->getId()].put(*eI);
    equipment.erase(uEI, equipment.end());
}

void Traveler::equip(Good &g) {
    // Equip the given good.
    auto &ss = g.getMaterial().getCombatStats();
    std::vector<int> pIs; // part ids used by this equipment
    pIs.reserve(ss.size());
    for (auto &s : ss)
        if (pIs.empty() or pIs.back() != s.partId)
            pIs.push_back(s.partId);
    for (auto pI : pIs)
        unequip(pI);
    // Take good out of goods.
    goods[g.getId()].take(g);
    // Put good in equipment container.
    equipment.push_back(g);
}

void Traveler::equip(int pI) {
    for (auto &e : equipment)
        for (auto &s : e.getMaterial().getCombatStats())
            if (s.partId == pI)
                return;
    if (pI == 2) {
        // Add left fist to equipment.
        Good f = Good(0, "fist");
        Material fM(0, "left");
        fM.setCombatStats({{1, 2, 1, 1, 0, {{1, 1, 1}}}, {2, 2, 0, 1, 1, {{1, 1, 1}}}});
        f.addMaterial(fM);
        equipment.push_back(f);
    } else if (pI == 3) {
        // Add right fist to equipment.
        Good f = Good(0, "fist");
        Material fM(0, "right");
        fM.setCombatStats({{1, 3, 1, 1, 0, {{1, 1, 1}}}, {2, 3, 0, 1, 1, {{1, 1, 1}}}});
        f.addMaterial(fM);
        equipment.push_back(f);
    }
}

std::vector<std::shared_ptr<Traveler>> Traveler::attackable() const {
    // Get a vector of attackable travelers.
    auto &forwardTravelers = fromTown->getTravelers(); // travelers traveling from the same town
    auto &backwardTravelers = toTown->getTravelers();  // travelers traveling from the destination town
    std::vector<std::shared_ptr<Traveler>> able;
    able.insert(able.end(), forwardTravelers.begin(), forwardTravelers.end());
    if (fromTown != toTown)
        able.insert(able.end(), backwardTravelers.begin(), backwardTravelers.end());
    if (not able.empty()) {
        // Eliminate travelers which are too far away or have reached town or are already fighting or are this traveler.
        able.erase(std::remove_if(able.begin(), able.end(),
                                  [this](std::shared_ptr<Traveler> t) {
                                      return t->toTown == t->fromTown or
                                             distSq(t->px, t->py) > Settings::getAttackDistSq() or t->target.lock() or
                                             not t->alive() or t == shared_from_this();
                                  }),
                   able.end());
    }
    return able;
}

void Traveler::attack(std::shared_ptr<Traveler> t) {
    target = t;
    t->target = shared_from_this();
    fightTime = 0;
    t->fightTime = 0;
    if (ai)
        choice = FightChoice::fight;
    else
        choice = FightChoice::none;
    if (t->ai)
        t->ai->autoChoose(t->goods, t->stats, [t] { return t->speed(); }, goods, stats, [this] { return speed(); },
                              choice);
    else
        t->choice = FightChoice::none;
}

void Traveler::loseTarget() {
    if (auto t = target.lock()) {
        t->target.reset();
        // Remove target from town so that it can't be attacked again.
        t->fromTown->removeTraveler(t);
        ;
    }
    target.reset();
}

void Traveler::updateFightBoxes(std::vector<std::unique_ptr<TextBox>> &bs) {
    std::vector<std::string> choiceText = bs[0]->getText();
    switch (choice) {
    case FightChoice::none:
        break;
    case FightChoice::fight:
        if (choiceText[0] == "Running...")
            choiceText[0] = "Running... Caught!";
        break;
    case FightChoice::run:
        choiceText[0] = "Running...";
        break;
    case FightChoice::yield:
        choiceText[0] = "Yielding...";
        break;
    }
    bs[0]->setText(choiceText);
    std::vector<std::string> statusText(7);
    statusText[0] = name + "'s Status";
    for (size_t i = 1; i < statusText.size(); ++i)
        statusText[i] = gameData->parts[i - 1] + ": " + gameData->statuses[parts[i - 1]];
    bs[1]->setText(statusText);
    auto t = target.lock();
    if (not t)
        return;
    statusText[0] = t->name + "'s Status";
    for (size_t i = 1; i < statusText.size(); ++i)
        statusText[i] = gameData->parts[i - 1] + ": " + gameData->statuses[t->parts[i - 1]];
    bs[2]->setText(statusText);
}

CombatHit Traveler::firstHit(Traveler &t, std::uniform_real_distribution<> &d) {
    // Find first hit of this traveler on t.
    std::array<int, 3> defense{};
    for (auto &e : t.equipment)
        for (auto &s : e.getMaterial().getCombatStats())
            for (size_t i = 0; i < defense.size(); ++i)
                defense[i] += s.defense[i] * t.stats[s.statId];
    CombatHit first = {std::numeric_limits<double>::max()};
    for (auto &e : equipment) {
        auto &ss = e.getMaterial().getCombatStats();
        if (ss.front().attack) {
            // e is a weapon
            int attack = 0, type = ss.front().type, speed = 0;
            for (auto &s : ss) {
                attack += s.attack * stats[s.statId];
                speed += s.speed * stats[s.statId];
            }
            auto cO = gameData->odds[type - 1];
            // Calculate number of swings before hit happens.
            double r = d(*Settings::getRng());
            double p = attack / cO.hitOdds / defense[type - 1];
            double time;
            if (p < 1)
                time = (log(r) / log(1 - p) + 1) / speed;
            else
                time = 1. / speed;
            if (time < first.time) {
                first.time = time;
                // Pick a random part.
                first.partId = d(*Settings::getRng()) * parts.size();
                // Start status at part's current status.
                first.status = t.parts[first.partId];
                r = d(*Settings::getRng());
                auto sCI = cO.statusChances.begin();
                while (r > 0 and sCI != cO.statusChances.end()) {
                    // Find status such that part becomes more damaged.
                    if (sCI->first > first.status) {
                        // This part is less damaged than the current status.
                        first.status = sCI->first;
                        // Subtract chance of this status from r.
                        r -= sCI->second;
                    }
                    ++sCI;
                }
                first.weapon = e.getMaterial().getName() + " " + e.getName();
            }
        }
    }
    return first;
}

void Traveler::useAmmo(double t) {
    for (auto &e : equipment) {
        int sId = e.getShoots();
        if (sId) {
            int speed = 0;
            for (auto &s : e.getMaterial().getCombatStats())
                speed += s.speed * stats[s.statId];
            int n = t * speed;
            goods[sId].use(n);
        }
    }
}

void Traveler::runFight(Traveler &t, int e, std::uniform_real_distribution<> &d) {
    // Fight the t.
    fightTime += e;
    // Prevent fight from happening twice.
    t.fightTime -= e;
    // Keep fighting until one side dies, runs, or yields or time runs out.
    while (alive() and t.alive() and choice == FightChoice::fight and t.choice == FightChoice::fight and fightTime > 0) {
        CombatHit ourFirst = firstHit(t, d), theirFirst = t.firstHit(*this, d);
        if (ourFirst.time < theirFirst.time) {
            // Our hit happens first.
            t.takeHit(ourFirst, *this);
            fightTime -= ourFirst.time;
            useAmmo(ourFirst.time);
            t.useAmmo(ourFirst.time);
        } else if (theirFirst.time < ourFirst.time) {
            // Their hit happens first.
            takeHit(theirFirst, t);
            fightTime -= theirFirst.time;
            useAmmo(theirFirst.time);
            t.useAmmo(theirFirst.time);
        } else {
            // Both hits happen at the same time.
            takeHit(theirFirst, t);
            t.takeHit(ourFirst, *this);
            fightTime -= ourFirst.time;
            useAmmo(ourFirst.time);
            t.useAmmo(ourFirst.time);
        }
        if (ai)
            ai->autoChoose(goods, stats, [this] { return speed(); }, t.goods, t.stats, [&t] { return t.speed(); },
                           choice);
        if (t.ai)
            t.ai->autoChoose(t.goods, t.stats, [&t] { return t.speed(); }, goods, stats, [this] { return speed(); },
                              choice);
    }
}

void Traveler::takeHit(const CombatHit &cH, Traveler &t) {
    // Apply the given combat hit from the given traveler to this traveler.
    parts[cH.partId] = cH.status;
    if (cH.status > 2) { // Part is too wounded to hold equipment.
        unequip(cH.partId);
        std::string logEntry = t.name + "'s " + cH.weapon + " strikes " + name + ". " + name + "'s " +
                               gameData->parts[cH.partId] + " has been " + gameData->statuses[cH.status] + ".";
        logText.push_back(logEntry);
        t.logText.push_back(logEntry);
    }
}

void Traveler::loot(Good &g, Traveler &t) {
    // Take the given good from the given traveler.
    int gId = g.getId();
    t.goods[gId].take(g);
    goods[gId].put(g);
}

void Traveler::startAI() {
    // Initialized variables for running a new AI.
    ai = std::make_unique<AI>(goods, toTown);
}

void Traveler::startAI(const Traveler &p) {
    // Starts an AI which imitates parameter's AI.
    ai = std::make_unique<AI>(*p.ai, goods, toTown);
}

void Traveler::runAI(int e) {
    // Run the AI for the elapsed time.
    ai->run(e, moving, offer, request, goods, equipment, toTown, target, stats, [this] { return netWeight(); },
            [this] { makeTrade(); }, [this](Good &e) { equip(e); }, [this] { return attackable(); },
            [this](std::shared_ptr<Traveler> t) { attack(t); }, [this](Town *t) { pickTown(t); });
}

void Traveler::update(int e) {
    // Move traveler toward destination and perform combat with target.
    if (toTown and moving) {
        double t = e / static_cast<double>(Settings::getDayLength());
        // Take a step toward town.
        double dlt = toTown->getLatitude() - latitude;
        double dlg = toTown->getLongitude() - longitude;
        double ds = dlt * dlt + dlg * dlg;
        if (ds > t * t) {
            // There remains more distance to travel.
            if (dlg) {
                double m = dlt / dlg;
                double dxs = t * t / (1 + m * m);
                double dys = t * t - dxs;
                latitude += copysign(sqrt(dys), dlt);
                longitude += copysign(sqrt(dxs), dlg);
            } else
                latitude += copysign(t, dlt);
        } else {
            // We have reached the destination.
            latitude = toTown->getLatitude();
            longitude = toTown->getLongitude();
            fromTown->removeTraveler(shared_from_this());
            toTown->addTraveler(shared_from_this());
            fromTown = toTown;
            moving = false;
            logText.push_back(name + " has arrived in the " +
                              gameData->populationAdjectives.lower_bound(toTown->getPopulation())->second + " " +
                              toTown->getNation()->getAdjective() + " " +
                              gameData->townTypeNouns[toTown->getTownType() - 1] + " of " + toTown->getName() + ".");
        }
    }
    if (auto t = target.lock()) {
        if (fightWon() and ai) {
            ai->autoLoot(goods, target, [this] { return netWeight(); }, [this](Good &g, Traveler &t) { loot(g, t); });
            loseTarget();
            return;
        }
        if (choice == FightChoice::fight) {
            static std::uniform_real_distribution<> dis(0, 1);
            double escapeChance;
            switch (t->choice) {
            case FightChoice::fight:
                runFight(*t, e, dis);
                break;
            case FightChoice::run:
                // Check if target escapes.
                escapeChance = Settings::getEscapeChance() * t->speed() / speed();
                if (dis(*Settings::getRng()) > escapeChance) {
                    // Target is caught, fight.
                    t->choice = FightChoice::fight;
                    runFight(*t, e, dis);
                    std::string logEntry = name + " catches up to " + t->name + ".";
                    logText.push_back(logEntry);
                    t->logText.push_back(logEntry);
                } else {
                    // Target escapes.
                    std::string logEntry = t->name + " has eluded " + name + ".";
                    logText.push_back(logEntry);
                    t->logText.push_back(logEntry);
                    loseTarget();
                }
                break;
            default:
                break;
            }
        }
    }
}

void Traveler::adjustAreas(const std::vector<std::unique_ptr<TextBox>> &bs, double d) {
    std::vector<MenuButton *> requestButtons;
    for (auto rBI = bs.begin() + requestButtonIndex; rBI != bs.end(); ++rBI)
        requestButtons.push_back(dynamic_cast<MenuButton *>(rBI->get()));
    toTown->adjustAreas(requestButtons, d);
}

void Traveler::adjustDemand(const std::vector<std::unique_ptr<TextBox>> &bs, double d) {
    std::vector<MenuButton *> requestButtons;
    for (auto rBI = bs.begin() + requestButtonIndex; rBI != bs.end(); ++rBI)
        requestButtons.push_back(dynamic_cast<MenuButton *>(rBI->get()));
    toTown->adjustDemand(requestButtons, d);
}

void Traveler::resetTown() { toTown->resetGoods(); }

void Traveler::toggleMaxGoods() { toTown->toggleMaxGoods(); }

flatbuffers::Offset<Save::Traveler> Traveler::save(flatbuffers::FlatBufferBuilder &b) {
    // Return a flatbuffers save object for this traveler.
    auto sName = b.CreateString(name);
    auto sLog = b.CreateVectorOfStrings(logText);
    auto sGoods =
        b.CreateVector<flatbuffers::Offset<Save::Good>>(goods.size(), [this, &b](size_t i) { return goods[i].save(b); });
    auto sStats = b.CreateVector(std::vector<short>(stats.begin(), stats.end()));
    auto sParts = b.CreateVector(std::vector<short>(parts.begin(), parts.end()));
    auto sEquipment = b.CreateVector<flatbuffers::Offset<Save::Good>>(equipment.size(),
                                                                      [this, &b](size_t i) { return equipment[i].save(b); });
    if (ai)
        return Save::CreateTraveler(b, sName, toTown->getId(), fromTown->getId(), nation->getId(), sLog, longitude,
                                    latitude, sGoods, sStats, sParts, sEquipment, ai->save(b), moving);
    else
        return Save::CreateTraveler(b, sName, toTown->getId(), fromTown->getId(), nation->getId(), sLog, longitude,
                                    latitude, sGoods, sStats, sParts, sEquipment, 0, moving);
}
