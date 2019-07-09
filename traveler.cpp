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

#include "traveler.hpp"

Traveler::Traveler(const std::string &n, Town *t, const GameData &gD)
    : name(n), toTown(t), fromTown(t), nation(t->getNation()), longitude(t->getLongitude()), latitude(t->getLatitude()),
      portion(1), gameData(gD), moving(false) {
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
    static std::uniform_int_distribution<unsigned int> dis(1, Settings::getStatMax());
    for (auto &s : stats)
        s = dis(Settings::getRng());
    // Set parts status to normal.
    parts.fill(0);
}

Traveler::Traveler(const Save::Traveler *t, std::vector<Town> &ts, const std::vector<Nation> &ns, const GameData &gD)
    : name(t->name()->str()), toTown(&ts[static_cast<size_t>(t->toTown() - 1)]),
      fromTown(&ts[static_cast<size_t>(t->fromTown() - 1)]), nation(&ns[static_cast<size_t>(t->nation() - 1)]),
      longitude(t->longitude()), latitude(t->latitude()), portion(1), gameData(gD), moving(t->moving()) {
    auto lLog = t->log();
    for (auto lLI = lLog->begin(); lLI != lLog->end(); ++lLI)
        logText.push_back(lLI->str());
    auto lGoods = t->goods();
    for (auto lGI = lGoods->begin(); lGI != lGoods->end(); ++lGI)
        goods.push_back(Good(*lGI));
    auto lStats = t->stats();
    for (size_t i = 0; i < stats.size(); ++i)
        stats[i] = lStats->Get(static_cast<unsigned int>(i));
    auto lParts = t->parts();
    for (size_t i = 0; i < parts.size(); ++i)
        parts[i] = lParts->Get(static_cast<unsigned int>(i));
    auto lEquipment = t->equipment();
    for (auto lEI = lEquipment->begin(); lEI != lEquipment->end(); ++lEI)
        equipment.push_back(Good(*lEI));
}

void Traveler::addToTown() { fromTown->addTraveler(shared_from_this()); }

double Traveler::netWeight() const {
    return std::accumulate(goods.begin(), goods.end(), static_cast<double>(stats[0]) * -16.,
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

std::string Traveler::portionString() const {
    // Return a string representing trade portion with trailing zeroes omitted.
    std::string tPS = std::to_string(portion);
    size_t lNZI = tPS.find_last_not_of('0');
    size_t dI = tPS.find('.');
    tPS.erase(lNZI == dI ? dI + 2 : lNZI + 1, std::string::npos);
    return tPS;
}

double Traveler::offerGood(const Good &g, const Material &m) {
    // Add the given good and material to offer. Return amount added based on portion.
    double amount = m.getAmount() * portion;
    if (not g.getSplit())
        amount = floor(amount);
    Good oG(g.getId(), g.getName(), amount, g.getMeasure());
    oG.addMaterial(Material(m.getId(), m.getName(), amount));
    offer.push_back(oG);
    return amount;
}

double Traveler::requestGood(const Good &g, const Material &m, double oV, int rC) {
    // Add the given good and material to request. Return excess amount of good.
    double excess = 0;
    // Calculate amount based on offer value divided by request count.
    double amount = m.getQuantity(oV / rC * Settings::getTownProfit(), excess);
    if (not g.getSplit())
        // Remove extra portion of goods that don't split.
        excess += modf(amount, &amount);
    Good rG(g.getId(), g.getName(), amount, g.getMeasure());
    rG.addMaterial(Material(m.getId(), m.getName(), amount));
    request.push_back(rG);
    return excess;
}

void Traveler::divideExcess(double e) {
    // Divide excess among offered goods.
    e /= static_cast<double>(offer.size());
    for (auto &of : offer) {
        auto &tM = toTown->getGood(of.getId()).getMaterial(of.getMaterial());
        double q = tM.getQuantity(e / Settings::getTownProfit());
        if (not goods[of.getId()].getSplit())
            q = floor(q);
        of.use(q);
    }
}

void Traveler::setPortion(double d) {
    portion = d;
    if (portion < 0)
        portion = 0;
    else if (portion > 1)
        portion = 1;
}

void Traveler::changePortion(double d) { setPortion(portion + d); }

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
    px = static_cast<int>(s * longitude) + ox;
    py = oy - static_cast<int>(s * latitude);
}

void Traveler::draw(SDL_Renderer *s) const {
    SDL_Color col;
    if (ai)
        col = Settings::getAIColor();
    else
        col = Settings::getPlayerColor();
    drawCircle(s, px, py, 1, col, true);
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

void Traveler::createTradeButtons(std::vector<std::unique_ptr<TextBox>> &bs, size_t &oBI, size_t &rBI) {
    // Create offer and request buttons for the player. Store request button index in parameter

    const SDL_Rect &sR = Settings::getScreenRect();
    const SDL_Color &fgr = nation->getForeground(), &bgr = nation->getBackground(),
                    &tFgr = toTown->getNation()->getForeground(), &tBgr = toTown->getNation()->getBackground();
    int tB = Settings::getTradeBorder(), tR = Settings::getTradeRadius(), tFS = Settings::getTradeFontSize(),
        gBXD = Settings::getGoodButtonXDivisor(), gBYD = Settings::getGoodButtonYDivisor();
    std::function<void()> f = [this, &bs, &oBI, &rBI] {
        updateTradeButtons(bs, oBI, rBI);
    }; // function to call when buttons are clicked
    // Create the offer buttons for the player.
    int left = sR.w / gBXD, right = sR.w / 2, top = sR.h / gBXD, dx = sR.w * 31 / gBXD, dy = sR.h * 31 / gBYD;
    SDL_Rect rt = {left, top, sR.w * 29 / gBXD, sR.h * 29 / gBYD};
    for (auto &g : getGoods()) {
        for (auto &m : g.getMaterials())
            if ((m.getAmount() >= 0.01 and g.getSplit()) or (m.getAmount() >= 1.)) {
                bs.push_back(m.button(true, g.getId(), g.getName(), g.getSplit(), rt, fgr, bgr, tB, tR, tFS, gameData, f));
                rt.x += dx;
                if (rt.x + rt.w >= right) {
                    rt.x = left;
                    rt.y += dy;
                }
            }
    }
    left = sR.w / 2 + sR.w / gBXD;
    right = sR.w - sR.w / gBXD;
    rt.x = left;
    rt.y = top;
    rBI = bs.size();
    for (auto &g : toTown->getGoods()) {
        for (auto &m : g.getMaterials())
            if ((m.getAmount() >= 0.01 and g.getSplit()) or (m.getAmount() >= 1.)) {
                bs.push_back(m.button(true, g.getId(), g.getName(), g.getSplit(), rt, tFgr, tBgr, tB, tR, tFS, gameData, f));
                rt.x += dx;
                if (rt.x + rt.w >= right) {
                    rt.x = left;
                    rt.y += dy;
                }
            }
    }
}

void Traveler::updateTradeButtons(std::vector<std::unique_ptr<TextBox>> &bs, size_t oBI, size_t rBI) {
    // Update the values shown on trade portion, offer, and request bs and set offer and request.
    offer.clear();
    request.clear();
    std::string bN;
    double offerValue = 0;
    auto rBBIt = bs.begin() + static_cast<std::vector<std::unique_ptr<TextBox>>::difference_type>(
                                  rBI); // iterator to first request button
    // Loop through all bs after cancel button and before first request button.
    for (auto oBIt = bs.begin() + static_cast<std::vector<std::unique_ptr<TextBox>>::difference_type>(oBI); oBIt != rBBIt;
         ++oBIt) {
        auto &g = getGood((*oBIt)->getId()); // good corresponding to oBIt
        auto &gMs = g.getMaterials();
        bN = (*oBIt)->getText()[0]; // first name on button
        auto mI = std::find_if(gMs.begin(), gMs.end(), [bN](const Material &m) {
            const std::string &mN = m.getName();
            return mN.substr(0, mN.find(' ')) == bN.substr(0, bN.find(' '));
        });
        if (mI != gMs.end()) {
            // mI is iterator to the material on oBIt
            mI->updateButton(g.getSplit(), 0, 0, oBIt->get());
            if ((*oBIt)->getClicked()) {
                double amount = offerGood(g, *mI);
                // Use town's version of same material to get price.
                auto &tM = toTown->getGood(g.getId()).getMaterial(*mI);
                offerValue += tM.getPrice(amount);
            }
        }
    }
    int requestCount =
        std::accumulate(rBBIt, bs.end(), 0, [](int c, const std::unique_ptr<TextBox> &rB) { return c + rB->getClicked(); });
    double excess = 0; // excess value of offer over value needed for request
    // Loop through request buttons.
    for (auto rBI = rBBIt; rBI != bs.end(); ++rBI) {
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
                // Convert material excess to value and add to overall excess.
                excess += mI->getPrice(requestGood(g, *mI, offerValue, requestCount));
            }
        }
    }
    divideExcess(excess);
}

std::unordered_map<Town *, std::vector<Good>>::iterator Traveler::createStorage(Town *t) {
    // Put a vector of goods for current town in storage map and return iterator to it.
    std::vector<Good> sGs;
    sGs.reserve(goods.size());
    for (auto &g : goods) {
        sGs.push_back(Good(g.getId(), g.getName(), g.getPerish(), g.getCarry(), g.getMeasure()));
        for (auto &m : g.getMaterials())
            sGs.back().addMaterial(Material(m.getId(), m.getName()));
    }
    return storage.emplace(t, sGs).first;
}

void Traveler::deposit(Good &g) {
    // Put the given good in storage in the current town.
    auto sI = storage.find(toTown);
    if (sI == storage.end())
        sI = createStorage(toTown);
    unsigned int gId = g.getId();
    goods[gId].take(g);
    sI->second[gId].put(g);
}

void Traveler::withdraw(Good &g) {
    // Take the given good from storage in the current town.
    auto sI = storage.find(toTown);
    if (sI == storage.end())
        sI = createStorage(toTown);
    unsigned int gId = g.getId();
    sI->second[gId].take(g);
    goods[gId].put(g);
}

void Traveler::refreshStorageButtons(std::vector<std::unique_ptr<TextBox>> &bs, const int &fB, size_t sBI) {
    bs.erase(bs.begin() + static_cast<std::vector<std::unique_ptr<TextBox>>::difference_type>(sBI), bs.end());
    createStorageButtons(bs, fB, sBI);
    if (fB > -1 and static_cast<size_t>(fB) < bs.size())
        bs[static_cast<size_t>(fB)]->focus();
}

void Traveler::createStorageButtons(std::vector<std::unique_ptr<TextBox>> &bs, const int &fB, size_t sBI) {
    // Create buttons for depositing and withdrawing goods to and from the current town.
    const SDL_Rect &sR = Settings::getScreenRect();
    const SDL_Color &fgr = nation->getForeground(), &bgr = nation->getBackground(),
                    &tFgr = toTown->getNation()->getForeground(), &tBgr = toTown->getNation()->getBackground();
    int tB = Settings::getTradeBorder(), tR = Settings::getTradeRadius(), tFS = Settings::getTradeFontSize(),
        gBXD = Settings::getGoodButtonXDivisor(), gBYD = Settings::getGoodButtonYDivisor();
    // Create buttons for depositing goods.
    int left = sR.w / gBXD, right = sR.w / 2, top = sR.h / gBXD, dx = sR.w * 31 / gBXD, dy = sR.h * 31 / gBYD;
    SDL_Rect rt = {left, top, sR.w * 29 / gBXD, sR.h * 29 / gBYD};
    for (auto &g : goods)
        for (auto &m : g.getMaterials()) {
            if (((m.getAmount() >= 0.01 and g.getSplit()) or (m.getAmount() >= 1))) {
                bs.push_back(m.button(true, g.getId(), g.getName(), g.getSplit(), rt, fgr, bgr, tB, tR, tFS, gameData,
                                      [this, &g, &m, &bs, &fB, sBI] {
                                          Good dG(g.getId(), g.getAmount() * portion);
                                          dG.addMaterial(Material(m.getId(), m.getAmount()));
                                          deposit(dG);
                                          refreshStorageButtons(bs, fB, sBI);
                                      }));

                rt.x += dx;
                if (rt.x + rt.w >= right) {
                    rt.x = left;
                    rt.y += dy;
                }
            }
        }
    left = sR.w / 2 + sR.w / gBXD;
    right = sR.w - sR.w / gBXD;
    rt.x = left;
    rt.y = top;
    auto sI = storage.find(toTown);
    if (sI == storage.end())
        sI = createStorage(toTown);
    for (auto &g : sI->second) {
        for (auto &m : g.getMaterials())
            if ((m.getAmount() >= 0.01 and g.getSplit()) or (m.getAmount() >= 1)) {
                bs.push_back(m.button(true, g.getId(), g.getName(), g.getSplit(), rt, tFgr, tBgr, tB, tR, tFS, gameData,
                                      [this, &g, &m, &bs, &fB, sBI] {
                                          Good dG(g.getId(), g.getAmount() * portion);
                                          dG.addMaterial(Material(m.getId(), m.getAmount()));
                                          withdraw(dG);
                                          refreshStorageButtons(bs, fB, sBI);
                                      }));
                rt.x += dx;
                if (rt.x + rt.w >= right) {
                    rt.x = left;
                    rt.y += dy;
                }
            }
    }
}



void Traveler::unequip(unsigned int pI) {
    // Unequip all equipment using the given part id.
    auto unused = [pI](const Good &e) {
        auto &eSs = e.getMaterial().getCombatStats();
        return std::find_if(eSs.begin(), eSs.end(), [pI](const CombatStat &s) { return s.partId == pI; }) == eSs.end();
    };
    auto uEI = std::partition(equipment.begin(), equipment.end(), unused);
    // Put equipment back in goods.
    for (auto eI = uEI; eI != equipment.end(); ++eI)
        if (eI->getAmount() > 0.)
            goods[eI->getId()].put(*eI);
    equipment.erase(uEI, equipment.end());
}

void Traveler::equip(Good &g) {
    // Equip the given good.
    auto &ss = g.getMaterial().getCombatStats();
    std::vector<unsigned int> pIs; // part ids used by this equipment
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

void Traveler::equip(unsigned int pI) {
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

void Traveler::refreshEquipButtons(std::vector<std::unique_ptr<TextBox>> &bs, const int &fB, size_t eBI) {
    bs.erase(bs.begin() + static_cast<std::vector<std::unique_ptr<TextBox>>::difference_type>(eBI), bs.end());
    createEquipButtons(bs, fB, eBI);
    if (fB > -1 and static_cast<size_t>(fB) < bs.size())
        bs[static_cast<size_t>(fB)]->focus();
}

void Traveler::createEquipButtons(std::vector<std::unique_ptr<TextBox>> &bs, const int &fB, size_t eBI) {
    // Create buttons for equipping equippables.
    const SDL_Rect &sR = Settings::getScreenRect();
    const SDL_Color &fgr = nation->getForeground(), &bgr = nation->getBackground();
    int eB = Settings::getEquipBorder(), eR = Settings::getEquipRadius(), eFS = Settings::getEquipFontSize();
    int gBXD = Settings::getGoodButtonXDivisor(), gBYD = Settings::getGoodButtonYDivisor();
    int left = sR.w / gBXD, top = sR.h / gBYD, dx = sR.w * 36 / gBXD, dy = sR.h * 31 / gBYD;
    SDL_Rect rt = {left, top, sR.w * 35 / gBXD, sR.h * 29 / gBYD};
    std::array<std::vector<Good>, 6> equippable; // array of vectors corresponding to parts that can hold equipment
    for (auto &g : goods)
        for (auto &m : g.getMaterials()) {
            auto &ss = m.getCombatStats();
            if (not ss.empty() and m.getAmount() >= 1) {
                // This good has combat stats and we have at least one of it.
                size_t i = ss.front().partId;
                Good e(g.getId(), g.getName(), 1);
                Material eM(m.getId(), m.getName(), 1);
                eM.setCombatStats(ss);
                e.addMaterial(eM);
                equippable[i].push_back(e);
            }
        }
    // Create buttons for equipping equipment.
    for (auto &eP : equippable) {
        for (auto &e : eP) {
            bs.push_back(e.getMaterial().button(false, e.getId(), e.getName(), e.getSplit(), rt, fgr, bgr, eB, eR, eFS,
                                                gameData, [this, e, &bs, &fB, eBI]() mutable {
                                                    equip(e);
                                                    // Refresh buttons.
                                                    refreshEquipButtons(bs, fB, eBI);
                                                }));
            rt.y += dy;
        }
        rt.y = top;
        rt.x += dx;
    }
    // Create buttons for unequipping equipment.
    rt.y = top;
    left = sR.w * 218 / gBXD;
    for (auto &e : getEquipment()) {
        auto &ss = e.getMaterial().getCombatStats();
        unsigned int pI = ss.front().partId;
        rt.x = left + static_cast<int>(pI) * dx;
        bs.push_back(e.getMaterial().button(false, e.getId(), e.getName(), e.getSplit(), rt, fgr, bgr, eB, eR, eFS, gameData,
                                            [this, pI, ss, &bs, &fB, eBI] {
                                                unequip(pI);
                                                // Equip fists.
                                                for (auto &s : ss)
                                                    equip(s.partId);
                                                // Refresh buttons.
                                                refreshEquipButtons(bs, fB, eBI);
                                            }));
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
    // Attack the given parameter traveler.
    target = t;
    t->target = shared_from_this();
    fightTime = 0;
    t->fightTime = 0;
    if (ai)
        choice = FightChoice::fight;
    else
        choice = FightChoice::none;
    if (t->ai)
        t->ai->autoChoose(
            t->goods, t->stats, [t] { return t->speed(); }, goods, stats, [this] { return speed(); }, choice);
    else
        t->choice = FightChoice::none;
}

void Traveler::createAttackButton(std::vector<std::unique_ptr<TextBox>> &bs, const std::function<void()> &sSF) {
    // Put attackable traveler names in names vector.
    const SDL_Rect &sR = Settings::getScreenRect();
    int bBB = Settings::getBigBoxBorder(), bBR = Settings::getBigBoxRadius(), fFS = Settings::getFightFontSize();
    const SDL_Color &fgr = nation->getForeground(), &bgr = nation->getBackground(), &hl = nation->getHighlight();
    auto able = attackable();       // vector of attackable travelers
    std::vector<std::string> names; // vector of names of attackable travelers
    names.reserve(able.size());
    std::transform(able.begin(), able.end(), std::back_inserter(names),
                   [](const std::shared_ptr<Traveler> t) { return t->getName(); });
    // Create attack button.
    SDL_Rect rt = {sR.w / 4, sR.h / 4, sR.w / 2, sR.h / 2};
    bs.push_back(std::make_unique<SelectButton>(rt, names, fgr, bgr, hl, bBB, bBR, fFS, [this, &bs, able, &sSF] {
        int i = bs.back()->getHighlightLine();
        if (i > -1) {
            attack(able[static_cast<size_t>(i)]);
            sSF();
        } else
            bs.back()->setClicked(false);
    }));
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

CombatHit Traveler::firstHit(Traveler &t, std::uniform_real_distribution<> &d) {
    // Find first hit of this traveler on t.
    std::array<int, 3> defense{};
    for (auto &e : t.equipment)
        for (auto &s : e.getMaterial().getCombatStats())
            for (size_t i = 0; i < defense.size(); ++i)
                defense[i] += s.defense[i] * t.stats[s.statId];
    CombatHit first = {std::numeric_limits<double>::max(), 0, 0, ""};
    for (auto &e : equipment) {
        auto &ss = e.getMaterial().getCombatStats();
        if (ss.front().attack) {
            // e is a weapon
            unsigned int attack = 0, type = ss.front().type, speed = 0;
            for (auto &s : ss) {
                attack += s.attack * stats[s.statId];
                speed += s.speed * stats[s.statId];
            }
            auto cO = gameData.odds[type - 1];
            // Calculate number of swings before hit happens.
            double r = d(Settings::getRng());
            double p = attack / cO.hitOdds / defense[type - 1];
            double time;
            if (p < 1)
                time = (log(r) / log(1 - p) + 1) / speed;
            else
                time = 1. / speed;
            if (time < first.time) {
                first.time = time;
                // Pick a random part.
                first.partId = static_cast<unsigned int>(d(Settings::getRng()) * static_cast<double>(parts.size()));
                // Start status at part's current status.
                first.status = t.parts[first.partId];
                r = d(Settings::getRng());
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
        unsigned int sId = e.getShoots();
        if (sId) {
            unsigned int speed = 0;
            for (auto &s : e.getMaterial().getCombatStats())
                speed += s.speed * stats[s.statId];
            unsigned int n = static_cast<unsigned int>(t * speed);
            goods[sId].use(n);
        }
    }
}

void Traveler::runFight(Traveler &t, unsigned int e, std::uniform_real_distribution<> &d) {
    // Fight t for e milliseconds.
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
            ai->autoChoose(
                goods, stats, [this] { return speed(); }, t.goods, t.stats, [&t] { return t.speed(); }, choice);
        if (t.ai)
            t.ai->autoChoose(
                t.goods, t.stats, [&t] { return t.speed(); }, goods, stats, [this] { return speed(); }, choice);
    }
}

void Traveler::takeHit(const CombatHit &cH, Traveler &t) {
    // Apply the given combat hit from the given traveler to this traveler.
    parts[cH.partId] = cH.status;
    if (cH.status > 2) { // Part is too wounded to hold equipment.
        unequip(cH.partId);
        std::string logEntry = t.name + "'s " + cH.weapon + " strikes " + name + ". " + name + "'s " +
                               gameData.parts[cH.partId] + " has been " + gameData.statuses[cH.status] + ".";
        logText.push_back(logEntry);
        t.logText.push_back(logEntry);
    }
}

void Traveler::createFightBoxes(std::vector<std::unique_ptr<TextBox>> &bs, bool &p) {
    // Create buttons and text boxes for combat.
    const SDL_Rect &sR = Settings::getScreenRect();
    SDL_Rect rt = {sR.w / 2, sR.h / 4, 0, 0};
    auto tgt = target.lock();
    std::vector<std::string> tx = {"Fighting " + tgt->getName() + "..."};
    bs.push_back(std::make_unique<TextBox>(rt, tx, nation->getForeground(), nation->getBackground(),
                                           Settings::getBigBoxBorder(), Settings::getBigBoxRadius(),
                                           Settings::getFightFontSize()));
    rt = {sR.w / 21, sR.h / 4, sR.w * 5 / 21, sR.h / 2};
    tx = {};
    bs.push_back(std::make_unique<TextBox>(rt, tx, nation->getForeground(), nation->getBackground(),
                                           Settings::getBigBoxBorder(), Settings::getBigBoxRadius(),
                                           Settings::getFightFontSize()));
    rt = {sR.w * 15 / 21, sR.h / 4, sR.w * 5 / 21, sR.h / 2};
    tx = {};
    bs.push_back(std::make_unique<TextBox>(rt, tx, tgt->nation->getForeground(), tgt->nation->getBackground(),
                                           Settings::getBigBoxBorder(), Settings::getBigBoxRadius(),
                                           Settings::getFightFontSize()));
    rt = {sR.w / 3, sR.h / 3, sR.w / 3, sR.h / 3};
    tx = {"Fight", "Run", "Yield"};
    bs.push_back(std::make_unique<SelectButton>(rt, tx, nation->getForeground(), nation->getBackground(),
                                                nation->getHighlight(), Settings::getBigBoxBorder(),
                                                Settings::getBigBoxRadius(), Settings::getFightFontSize(), [this, &bs, &p] {
                                                    int choice = bs[kFightChoiceIndex]->getHighlightLine();
                                                    if (choice > -1) {
                                                        choose(static_cast<FightChoice>(choice));
                                                        p = false;
                                                    }
                                                }));
}

void Traveler::updateFightBoxes(std::vector<std::unique_ptr<TextBox>> &bs) {
    // Update TextBox objects for fight user interface.
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
        statusText[i] = gameData.parts[i - 1] + ": " + gameData.statuses[getPart(i - 1)];
    bs[1]->setText(statusText);
    auto tgt = target.lock();
    if (not tgt)
        return;
    statusText[0] = tgt->getName() + "'s Status";
    for (size_t i = 1; i < statusText.size(); ++i)
        statusText[i] = gameData.parts[i - 1] + ": " + gameData.statuses[tgt->getPart(i - 1)];
    bs[2]->setText(statusText);
}

void Traveler::loot(Good &g, Traveler &t) {
    // Take the given good from the given traveler.
    unsigned int gId = g.getId();
    t.goods[gId].take(g);
    goods[gId].put(g);
}

void Traveler::loot(Traveler &t) {
    for (auto g : t.goods)
        loot(g, t);
}

void Traveler::refreshLootButtons(std::vector<std::unique_ptr<TextBox>> &bs, const int &fB, size_t lBI) {
    bs.erase(bs.begin() + static_cast<std::vector<std::unique_ptr<TextBox>>::difference_type>(lBI), bs.end());
    createLootButtons(bs, fB, lBI);
    if (fB > -1 and static_cast<size_t>(fB) < bs.size())
        bs[static_cast<size_t>(fB)]->focus();
}

void Traveler::createLootButtons(std::vector<std::unique_ptr<TextBox>> &bs, const int &fB, size_t lBI) {
    const SDL_Rect &sR = Settings::getScreenRect();
    int tB = Settings::getTradeBorder(), tR = Settings::getTradeRadius(), tFS = Settings::getTradeFontSize(),
        gBXD = Settings::getGoodButtonXDivisor(), gBYD = Settings::getGoodButtonYDivisor();
    auto &tgt = *target.lock();
    const SDL_Color &fgr = nation->getForeground(), &bgr = nation->getBackground(), &tFgr = tgt.nation->getForeground(),
                    &tBgr = tgt.nation->getBackground();
    int left = sR.w / gBXD, right = sR.w / 2;
    int top = sR.h / gBYD;
    int dx = sR.w * 31 / gBXD, dy = sR.h * 31 / gBYD;
    SDL_Rect rt = {left, top, sR.w * 29 / gBXD, sR.h * 29 / gBYD};
    for (auto &g : goods)
        for (auto &m : g.getMaterials()) {
            if ((m.getAmount() >= 0.01 and g.getSplit()) or (m.getAmount() >= 1)) {
                bs.push_back(m.button(true, g.getId(), g.getName(), g.getSplit(), rt, fgr, bgr, tB, tR, tFS, gameData,
                                      [this, &g, &m, &tgt, &bs, &fB, lBI] {
                                          Good lG(g.getId(), m.getAmount());
                                          lG.addMaterial(Material(m.getId(), m.getAmount()));
                                          tgt.loot(lG, *this);
                                          refreshLootButtons(bs, fB, lBI);
                                      }));
                rt.x += dx;
                if (rt.x + rt.w >= right) {
                    rt.x = left;
                    rt.y += dy;
                }
            }
        }
    left = sR.w / 2 + sR.w / gBXD;
    right = sR.w - sR.w / gBXD;
    rt.x = left;
    rt.y = top;
    for (auto &g : tgt.goods)
        for (auto &m : g.getMaterials()) {
            if ((m.getAmount() >= 0.01 and g.getSplit()) or (m.getAmount() >= 1)) {
                bs.push_back(m.button(true, g.getId(), g.getName(), g.getSplit(), rt, tFgr, tBgr, tB, tR, tFS, gameData,
                                      [this, &g, &m, &tgt, &bs, &fB, lBI] {
                                          Good lG(g.getId(), m.getAmount());
                                          lG.addMaterial(Material(m.getId(), m.getAmount()));
                                          loot(lG, tgt);
                                          refreshLootButtons(bs, fB, lBI);
                                      }));
                rt.x += dx;
                if (rt.x + rt.w >= right) {
                    rt.x = left;
                    rt.y += dy;
                }
            }
        }
}

void Traveler::startAI() {
    // Initialized variables for running a new AI.
    ai = std::make_unique<AI>(goods, toTown);
}

void Traveler::startAI(const Traveler &p) {
    // Starts an AI which imitates parameter's AI.
    ai = std::make_unique<AI>(*p.ai, goods, toTown);
}

void Traveler::runAI(unsigned int e) {
    // Run the AI for the elapsed time.
    ai->run(
        e, moving, offer, request, goods, equipment, toTown, target, stats, [this] { return netWeight(); },
        [this] { makeTrade(); }, [this](Good &e) { equip(e); }, [this] { return attackable(); },
        [this](std::shared_ptr<Traveler> t) { attack(t); }, [this](Town *t) { pickTown(t); });
}

void Traveler::update(unsigned int e) {
    // Move traveler toward destination and perform combat with target.
    if (toTown and moving) {
        double t = static_cast<double>(e) / static_cast<double>(Settings::getDayLength());
        // Take a step toward town.
        double dlt = toTown->getLatitude() - latitude;
        double dlg = toTown->getLongitude() - longitude;
        double ds = dlt * dlt + dlg * dlg;
        if (ds > t * t) {
            // There remains more distance to travel.
            if (dlg != 0.) {
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
                              gameData.populationAdjectives.lower_bound(toTown->getPopulation())->second + " " +
                              toTown->getNation()->getAdjective() + " " + gameData.townTypeNouns[toTown->getTownType() - 1] +
                              " of " + toTown->getName() + ".");
        }
    }
    if (auto t = target.lock()) {
        if (fightWon() and ai) {
            ai->autoLoot(
                goods, target, [this] { return netWeight(); }, [this](Good &g, Traveler &t) { loot(g, t); });
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
                if (dis(Settings::getRng()) > escapeChance) {
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

void Traveler::adjustAreas(const std::vector<std::unique_ptr<TextBox>> &bs, size_t i, double d) {
    std::vector<MenuButton *> requestButtons;
    for (auto rBI = bs.begin() + i; rBI != bs.end(); ++rBI)
        requestButtons.push_back(dynamic_cast<MenuButton *>(rBI->get()));
    toTown->adjustAreas(requestButtons, d);
}

void Traveler::adjustDemand(const std::vector<std::unique_ptr<TextBox>> &bs, size_t i, double d) {
    // Adjust demand for goods in current town and show new prices on buttons.
    std::vector<MenuButton *> requestButtons;
    for (auto rBI = bs.begin() + i; rBI != bs.end(); ++rBI)
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
    auto sStats = b.CreateVector(std::vector<unsigned int>(stats.begin(), stats.end()));
    auto sParts = b.CreateVector(std::vector<unsigned int>(parts.begin(), parts.end()));
    auto sEquipment = b.CreateVector<flatbuffers::Offset<Save::Good>>(equipment.size(),
                                                                      [this, &b](size_t i) { return equipment[i].save(b); });
    if (ai)
        return Save::CreateTraveler(b, sName, toTown->getId(), fromTown->getId(), nation->getId(), sLog, longitude, latitude,
                                    sGoods, sStats, sParts, sEquipment, ai->save(b), moving);
    else
        return Save::CreateTraveler(b, sName, toTown->getId(), fromTown->getId(), nation->getId(), sLog, longitude, latitude,
                                    sGoods, sStats, sParts, sEquipment, 0, moving);
}
