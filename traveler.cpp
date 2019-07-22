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

Traveler::Traveler(const std::string& n, Town* t, const GameData& gD)
    : name(n), toTown(t), fromTown(t), nation(t->getNation()), longitude(t->getLongitude()), latitude(t->getLatitude()),
      moving(false), portion(1), properties(gD.townCount), gameData(gD) {
    // Copy goods vector from nation.
    const std::vector<Good>& gs = t->getNation()->getGoods();
    goods.reserve(gs.size());
    for (auto& g : gs) { goods.push_back(Good(g)); }
    // Create starting goods.
    goods[55].create(0.75);
    goods[59].create(2);
    // Equip fists.
    equip(2);
    equip(3);
    // Randomize stats.
    static std::uniform_int_distribution<unsigned int> dis(1, Settings::getStatMax());
    for (auto& s : stats) s = dis(Settings::getRng());
    // Set parts status to normal.
    parts.fill(0);
}

Traveler::Traveler(const Save::Traveler* t, std::vector<Town>& ts, const std::vector<Nation>& ns, const GameData& gD)
    : name(t->name()->str()), toTown(&ts[static_cast<size_t>(t->toTown() - 1)]),
      fromTown(&ts[static_cast<size_t>(t->fromTown() - 1)]), nation(&ns[static_cast<size_t>(t->nation() - 1)]),
      longitude(t->longitude()), latitude(t->latitude()), moving(t->moving()), portion(1), gameData(gD) {
    auto lLog = t->log();
    for (auto lLI = lLog->begin(); lLI != lLog->end(); ++lLI) logText.push_back(lLI->str());
    auto lGoods = t->goods();
    for (auto lGI = lGoods->begin(); lGI != lGoods->end(); ++lGI) goods.push_back(Good(*lGI));
    auto lProperties = t->properties();
    for (auto lPI = lProperties->begin(); lPI != lProperties->end(); ++lPI) {
        properties.push_back(Property());
        auto lStorage = lPI->storage();
        for (auto lSI = lStorage->begin(); lSI != lStorage->end(); ++lSI)
            properties.back().storage.push_back(Good(*lSI));
        auto lBusinesses = lPI->businesses();
        for (auto lBI = lBusinesses->begin(); lBI != lBusinesses->end(); ++lBI)
            properties.back().businesses.push_back(Business(*lBI));
    }
    auto lStats = t->stats();
    for (size_t i = 0; i < stats.size(); ++i) stats[i] = lStats->Get(static_cast<unsigned int>(i));
    auto lParts = t->parts();
    for (size_t i = 0; i < parts.size(); ++i) parts[i] = lParts->Get(static_cast<unsigned int>(i));
    auto lEquipment = t->equipment();
    for (auto lEI = lEquipment->begin(); lEI != lEquipment->end(); ++lEI) equipment.push_back(Good(*lEI));
    // Copy good image pointers from nation's goods to traveler's goods.
    auto& nGs = nation->getGoods();
    for (size_t i = 0; i < nGs.size(); ++i) {
        auto& nGMs = nGs[i].getMaterials();
        for (size_t j = 0; j < nGMs.size(); ++j) goods[i].setImage(j, nGMs[j].getImage());
    }
}

void Traveler::addToTown() { fromTown->addTraveler(shared_from_this()); }

double Traveler::netWeight() const {
    return std::accumulate(goods.begin(), goods.end(), static_cast<double>(stats[0]) * kTravelerCarry,
                           [](double d, Good g) { return d + g.getCarry() * g.getAmount(); });
}

std::forward_list<Town*> Traveler::pathTo(const Town* t) const {
    // return forward list of towns on shortest path to t, excluding current town
    std::forward_list<Town*> path;
    // the town which each town can be most efficiently reached from
    std::unordered_map<Town*, Town*> from;
    // distance along route to each town
    std::unordered_map<Town*, double> distTo({{toTown, 0}});
    // estimated distance along route through each town to goal
    std::unordered_map<Town*, double> distEst({{toTown, toTown->dist(t)}});
    // set of towns already evaluated
    std::unordered_set<Town*> closed;
    auto shortest = [&distEst](Town* m, Town* n) {
        double c = distEst[m];
        double d = distEst[n];
        if (c == d) return m->getId() < n->getId();
        return c < d;
    };
    // set of discovered towns not yet evaluated sorted so that shortest is first
    std::set<Town*, decltype(shortest)> open(shortest);
    open.insert(toTown);
    while (!open.empty()) {
        // find current town with lowest distance
        Town* current = *open.begin();
        const std::vector<Town*>& neighbors = current->getNeighbors();
        if (current == t) {
            while (from.count(current)) {
                path.push_front(current);
                current = from[current];
            }
            return path;
        }
        open.erase(open.begin());
        closed.insert(current);
        for (auto& n : neighbors) {
            // ignore town if already explored
            if (closed.count(n)) continue;
            // find distance to n through current
            double dT = distTo[current] + current->dist(n);
            // check if distance through current is less than previous shortest
            // path to n
            if (!distTo.count(n) || dT < distTo[n]) {
                from[n] = current;
                distTo[n] = dT;
                distEst[n] = dT + n->dist(t);
            }
            // add undiscovered node to open set
            if (!open.count(n)) open.insert(n);
        }
    }
    return path;
}

double Traveler::distSq(int x, int y) const { return (px - x) * (px - x) + (py - y) * (py - y); }

double Traveler::pathDist(const Town* t) const {
    // return the distance to t along shortest path
    const std::forward_list<Town*>& path = pathTo(t);
    double dist = 0;
    const Town* m = toTown;
    for (auto& n : path) {
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

void Traveler::setPortion(double d) {
    portion = d;
    if (portion < 0)
        portion = 0;
    else if (portion > 1)
        portion = 1;
}

void Traveler::changePortion(double d) { setPortion(portion + d); }

void Traveler::pickTown(const Town* t) {
    if (netWeight() > 0 /* || moving*/) return;
    const std::forward_list<Town*>& path = pathTo(t);
    if (path.empty() || path.front() == toTown) return;
    moving = true;
    toTown = path.front();
}

void Traveler::place(int ox, int oy, double s) {
    px = static_cast<int>(s * longitude) + ox;
    py = oy - static_cast<int>(s * latitude);
}

void Traveler::draw(SDL_Renderer* s) const {
    SDL_Color col;
    if (ai)
        col = Settings::getAIColor();
    else
        col = Settings::getPlayerColor();
    drawCircle(s, px, py, 1, col, true);
}

void Traveler::clearTrade() {
    offer.clear();
    request.clear();
}

void Traveler::makeTrade() {
    if (not(offer.empty() || request.empty())) {
        std::string logEntry = name + " trades ";
        for (auto gI = offer.begin(); gI != offer.end(); ++gI) {
            if (gI != offer.begin()) {
                // This is not the first good in offer.
                if (offer.size() != 2) logEntry += ", ";
                if (gI + 1 == offer.end())
                    // This is the last good in offer.
                    logEntry += " && ";
            }
            goods[gI->getId()].take(*gI);
            toTown->put(*gI);
            logEntry += gI->logText();
        }
        logEntry += " for ";
        for (auto gI = request.begin(); gI != request.end(); ++gI) {
            if (gI != request.begin()) {
                // This is not the first good in request.
                if (request.size() != 2) logEntry += ", ";
                if (gI + 1 == request.end())
                    // This is the last good in request.
                    logEntry += " && ";
            }
            toTown->take(*gI);
            goods[gI->getId()].put(*gI);
            logEntry += gI->logText();
        }
        logEntry += " in " + toTown->getName() + ".";
        logText.push_back(logEntry);
    }
}

void Traveler::divideExcess(double ec) {
    // Divide excess value among amounts of offered goods.
    ec /= static_cast<double>(offer.size());
    for (auto& of : offer) {
        // Convert value to quantity of this good.
        auto& tM = toTown->getGood(of.getId()).getMaterial(of.getMaterial()); // town material
        double q = tM.getQuantity(ec / Settings::getTownProfit());
        if (!goods[of.getId()].getSplit()) q = floor(q);
        // Reduce quantity.
        of.use(q);
    }
}

void Traveler::createTradeButtons(std::vector<std::unique_ptr<TextBox>>& bs, size_t& oBI, size_t& rBI, Printer& pr) {
    // Create offer and request buttons for the player. Store request button index in parameter

    const SDL_Rect& sR = Settings::getScreenRect();
    const SDL_Color &fgr = nation->getForeground(), &bgr = nation->getBackground(),
                    &tFgr = toTown->getNation()->getForeground(), &tBgr = toTown->getNation()->getBackground();
    int tB = Settings::getTradeBorder(), tR = Settings::getTradeRadius(), tFS = Settings::getTradeFontSize();
    std::function<void()> fn = [this, &bs, &oBI, &rBI] { updateTradeButtons(bs, oBI, rBI); }; // function to call when buttons are clicked
    // Create the offer buttons for the player.
    int left = sR.w / kGoodButtonXDivisor, right = sR.w / 2, top = sR.h / kGoodButtonXDivisor,
        dx = sR.w * kGoodButtonSpaceMultiplier / kGoodButtonXDivisor, dy = sR.h * kGoodButtonSpaceMultiplier / kGoodButtonYDivisor;
    SDL_Rect rt = {left, top, sR.w * kGoodButtonSizeMultiplier / kGoodButtonXDivisor,
                   sR.h * kGoodButtonSizeMultiplier / kGoodButtonYDivisor};
    for (auto& g : getGoods()) {
        for (auto& m : g.getMaterials())
            if ((m.getAmount() >= 0.01 && g.getSplit()) || (m.getAmount() >= 1.)) {
                bs.push_back(g.button(true, m, rt, fgr, bgr, tB, tR, tFS, pr, fn));
                rt.x += dx;
                if (rt.x + rt.w >= right) {
                    rt.x = left;
                    rt.y += dy;
                }
            }
    }
    left = sR.w / 2 + sR.w / kGoodButtonXDivisor;
    right = sR.w - sR.w / kGoodButtonXDivisor;
    rt.x = left;
    rt.y = top;
    rBI = bs.size();
    for (auto& g : toTown->getGoods()) {
        for (auto& m : g.getMaterials())
            if ((m.getAmount() >= 0.01 && g.getSplit()) || (m.getAmount() >= 1.)) {
                bs.push_back(g.button(true, m, rt, tFgr, tBgr, tB, tR, tFS, pr, fn));
                rt.x += dx;
                if (rt.x + rt.w >= right) {
                    rt.x = left;
                    rt.y += dy;
                }
            }
    }
}

void Traveler::updateTradeButtons(std::vector<std::unique_ptr<TextBox>>& bs, size_t oBI, size_t rBI) {
    // Update the values shown on offer and request boxes and set offer and request.
    std::string bN;
    offer.clear();
    request.clear();
    double offerValue = 0.;
    auto rBBIt = bs.begin() + static_cast<std::vector<std::unique_ptr<TextBox>>::difference_type>(rBI); // iterator to first request button
    // Loop through all boxes after cancel button and before first request button.
    for (auto bIt = bs.begin() + static_cast<std::vector<std::unique_ptr<TextBox>>::difference_type>(oBI); bIt != rBBIt; ++bIt) {
        auto& g = goods[(*bIt)->getId()]; // good corresponding to bIt
        auto& gMs = g.getMaterials();     // g's materials
        bN = (*bIt)->getText()[0];        // name of material and good on button
        auto mI = std::find_if(gMs.begin(), gMs.end(), [bN](const Material& m) {
            const std::string& mN = m.getName(); // material name
            // Find material such that first word of material name matches first word on button.
            return mN.substr(0, mN.find(' ')) == bN.substr(0, bN.find(' '));
        });
        if (mI != gMs.end()) {
            // mI is iterator to the material on bIt
            mI->updateButton(g.getSplit(), bIt->get());
            if ((*bIt)->getClicked()) {
                // Button was clicked, so add corresponding good to offer.
                double amount = mI->getAmount() * portion;
                if (!g.getSplit()) amount = floor(amount);
                auto& tM = toTown->getGood(g.getId()).getMaterial(*mI);
                double p = tM.getPrice(amount);
                if (p > 0.) {
                    offerValue += p;
                    offer.push_back(Good(g.getId(), g.getName(), amount, g.getMeasure()));
                    offer.back().addMaterial(Material(mI->getId(), mI->getName(), amount));
                } else
                    // Good is worthless in this town, don't allow it to be offered.
                    (*bIt)->setClicked(false);
            }
        }
    }
    unsigned int requestCount = std::accumulate(
        rBBIt, bs.end(), 0u, [](unsigned int c, const std::unique_ptr<TextBox>& rB) { return c + rB->getClicked(); });
    double excess = 0; // excess value of offer over value needed for request
    // Loop through request buttons.
    for (auto bIt = rBBIt; bIt != bs.end(); ++bIt) {
        auto& g = toTown->getGood((*bIt)->getId()); // good in town corresponding to bIt
        auto& gMs = g.getMaterials();
        bN = (*bIt)->getText()[0];
        auto mI = std::find_if(gMs.begin(), gMs.end(), [bN](const Material& m) {
            const std::string& mN = m.getName();
            // Find material such that first word of name matches first word on button.
            return mN.substr(0, mN.find(' ')) == bN.substr(0, bN.find(' '));
        });
        if (mI != gMs.end()) {
            // mI is iterator to the material on bIt.
            if (offer.size()) {
                mI->updateButton(g.getSplit(), offerValue, std::max(1u, requestCount), bIt->get());
                if ((*bIt)->getClicked() && offerValue > 0.) {
                    double mE = 0; // excess quantity of this material
                    double amount = mI->getQuantity(offerValue / requestCount * Settings::getTownProfit(), mE);
                    if (!g.getSplit())
                        // Remove extra portion of goods that don't split.
                        mE += modf(amount, &amount);
                    // Convert material excess to value and add to overall excess.
                    excess += mI->getPrice(mE);
                    request.push_back(Good(g.getId(), g.getName(), amount, g.getMeasure()));
                    request.back().addMaterial(Material(mI->getId(), mI->getName(), amount));
                }
            } else
                mI->updateButton(g.getSplit(), bIt->get());
        }
    }
    if (offer.size()) divideExcess(excess);
}

void Traveler::deposit(Good& g) {
    // Put the given good in storage in the current town.
    auto& storage = properties[toTown->getId() - 1].storage;
    if (storage.empty()) {
        // Storage has not been created yet.
        auto& townGoods = toTown->getGoods(); // goods from the town
        storage.reserve(townGoods.size());
        for (auto& g : townGoods) {
            // Copy goods from town, omitting amounts.
            storage.push_back(Good(g.getId(), g.getName(), g.getPerish(), g.getCarry(), g.getMeasure()));
            for (auto& m : g.getMaterials()) storage.back().addMaterial(Material(m.getId(), m.getName(), m.getImage()));
        }
    }
    unsigned int gId = g.getId();
    goods[gId].take(g);
    storage[gId].put(g);
}

void Traveler::withdraw(Good& g) {
    // Take the given good from storage in the current town.
    auto& storage = properties[toTown->getId() - 1].storage;
    if (storage.empty())
        // Cannot withdraw from empty storage.
        return;
    unsigned int gId = g.getId();
    storage[gId].take(g);
    goods[gId].put(g);
}

void Traveler::refreshStorageButtons(std::vector<std::unique_ptr<TextBox>>& bs, const int& fB, size_t sBI, Printer& pr) {
    // Delete and re-create storage buttons to reflect changes.
    bs.erase(bs.begin() + static_cast<std::vector<std::unique_ptr<TextBox>>::difference_type>(sBI), bs.end());
    createStorageButtons(bs, fB, sBI, pr);
    // Change border on the focused box.
    if (fB > -1 && static_cast<size_t>(fB) < bs.size()) bs[static_cast<size_t>(fB)]->changeBorder(kBoxDBS);
}

void Traveler::createStorageButtons(std::vector<std::unique_ptr<TextBox>>& bs, const int& fB, size_t sBI, Printer& pr) {
    // Create buttons for depositing and withdrawing goods to and from the current town.
    const SDL_Rect& sR = Settings::getScreenRect();
    const SDL_Color &fgr = nation->getForeground(), &bgr = nation->getBackground(),
                    &tFgr = toTown->getNation()->getForeground(), &tBgr = toTown->getNation()->getBackground();
    int tB = Settings::getTradeBorder(), tR = Settings::getTradeRadius(), tFS = Settings::getTradeFontSize();
    // Create buttons for depositing goods.
    int left = sR.w / kGoodButtonXDivisor, right = sR.w / 2, top = sR.h / kGoodButtonXDivisor,
        dx = sR.w * kGoodButtonSpaceMultiplier / kGoodButtonXDivisor, dy = sR.h * kGoodButtonSpaceMultiplier / kGoodButtonYDivisor;
    SDL_Rect rt = {left, top, sR.w * kGoodButtonSizeMultiplier / kGoodButtonXDivisor,
                   sR.h * kGoodButtonSizeMultiplier / kGoodButtonYDivisor};
    for (auto& g : goods)
        for (auto& m : g.getMaterials()) {
            if (((m.getAmount() >= 0.01 && g.getSplit()) || (m.getAmount() >= 1))) {
                bs.push_back(g.button(true, m, rt, fgr, bgr, tB, tR, tFS, pr, [this, &g, &m, &bs, &fB, sBI, &pr] {
                    Good dG(g.getId(), g.getAmount() * portion);
                    dG.addMaterial(Material(m.getId(), m.getAmount()));
                    deposit(dG);
                    refreshStorageButtons(bs, fB, sBI, pr);
                }));

                rt.x += dx;
                if (rt.x + rt.w >= right) {
                    rt.x = left;
                    rt.y += dy;
                }
            }
        }
    left = sR.w / 2 + sR.w / kGoodButtonXDivisor;
    right = sR.w - sR.w / kGoodButtonXDivisor;
    rt.x = left;
    rt.y = top;
    auto& storage = properties[toTown->getId() - 1].storage;
    for (auto& g : storage) {
        for (auto& m : g.getMaterials())
            if ((m.getAmount() >= 0.01 && g.getSplit()) || (m.getAmount() >= 1)) {
                bs.push_back(g.button(true, m, rt, tFgr, tBgr, tB, tR, tFS, pr, [this, &g, &m, &bs, &fB, sBI, &pr] {
                    Good dG(g.getId(), g.getAmount() * portion);
                    dG.addMaterial(Material(m.getId(), m.getAmount()));
                    withdraw(dG);
                    refreshStorageButtons(bs, fB, sBI, pr);
                }));
                rt.x += dx;
                if (rt.x + rt.w >= right) {
                    rt.x = left;
                    rt.y += dy;
                }
            }
    }
}

void Traveler::build(const Business& bsn, double a) {
    // Build the given area of the given business. Check requirements before
    // calling. Determine if business exists in property.
    auto& oBsns = properties[toTown->getId() - 1].businesses;
    auto oBsnIt = std::lower_bound(oBsns.begin(), oBsns.end(), bsn);
    if (*oBsnIt == bsn) {
        // Business already exists, grow it.
        oBsnIt->takeRequirements(goods, a);
        oBsnIt->changeArea(a);
    } else {
        // Insert new business into properties and set its area.
        auto nwBsns = oBsns.insert(oBsnIt, Business(bsn));
        nwBsns->takeRequirements(goods, a);
        nwBsns->setArea(a);
    }
}

void Traveler::demolish(const Business& bsn, double a) {
    // Demolish the given area of the given business. Check area before calling.
    auto& oBsns = properties[toTown->getId() - 1].businesses;
    auto oBsnIt = std::lower_bound(oBsns.begin(), oBsns.end(), bsn);
    oBsnIt->changeArea(-a);
    oBsnIt->reclaim(goods, a);
    if (oBsnIt->getArea() == 0.) oBsns.erase(oBsnIt);
}

void Traveler::createManageButtons(std::vector<std::unique_ptr<TextBox>>& bs, Printer& pr) {
    // Create buttons for managing businesses.
    const SDL_Rect& sR = Settings::getScreenRect();
    const SDL_Color &fgr = nation->getForeground(), &bgr = nation->getBackground(),
                    &tFgr = toTown->getNation()->getForeground(), &tBgr = toTown->getNation()->getBackground();
    int tB = Settings::getTradeBorder(), tR = Settings::getTradeRadius(), tFS = Settings::getTradeFontSize();
    // Create buttons for demolishing businesses.
    std::vector<Business>& oBsns = properties[toTown->getId() - 1].businesses;
    int left = sR.w / kBusinessButtonXDivisor, right = sR.w / 2, top = sR.h / kBusinessButtonYDivisor,
        dx = sR.w * kBusinessButtonSpaceMultiplier / kBusinessButtonXDivisor,
        dy = sR.h * kBusinessButtonSpaceMultiplier / kBusinessButtonYDivisor;
    SDL_Rect rt = {left, top, sR.w * kBusinessButtonSizeMultiplier / kBusinessButtonXDivisor,
                   sR.h * kBusinessButtonSizeMultiplier / kBusinessButtonYDivisor};
    for (auto& bsn : oBsns) {
        bs.push_back(bsn.button(true, rt, fgr, bgr, tB, tR, tFS, pr, [this, &bsn, &pr] {

        }));

        rt.x += dx;
        if (rt.x + rt.w >= right) {
            rt.x = left;
            rt.y += dy;
        }
    }
    const std::vector<Business>& tBsns = toTown->getBusinesses();
    left = sR.w / 2 + sR.w / kGoodButtonXDivisor;
    right = sR.w - sR.w / kGoodButtonXDivisor;
    rt.x = left;
    rt.y = top;
    for (auto& bsn : tBsns) {
        bs.push_back(bsn.button(true, rt, tFgr, tBgr, tB, tR, tFS, pr, [this, &bsn, &pr] {

        }));

        rt.x += dx;
        if (rt.x + rt.w >= right) {
            rt.x = left;
            rt.y += dy;
        }
    }
}

void Traveler::unequip(unsigned int pI) {
    // Unequip all equipment using the given part id.
    auto unused = [pI](const Good& e) {
        auto& eSs = e.getMaterial().getCombatStats();
        return std::find_if(eSs.begin(), eSs.end(), [pI](const CombatStat& s) { return s.partId == pI; }) == eSs.end();
    };
    auto uEI = std::partition(equipment.begin(), equipment.end(), unused);
    // Put equipment back in goods.
    for (auto eI = uEI; eI != equipment.end(); ++eI)
        if (eI->getAmount() > 0.) goods[eI->getId()].put(*eI);
    equipment.erase(uEI, equipment.end());
}

void Traveler::equip(Good& g) {
    // Equip the given good.
    auto& ss = g.getMaterial().getCombatStats();
    std::vector<unsigned int> pIs; // part ids used by this equipment
    pIs.reserve(ss.size());
    for (auto& s : ss)
        if (pIs.empty() || pIs.back() != s.partId) pIs.push_back(s.partId);
    for (auto pI : pIs) unequip(pI);
    // Take good out of goods.
    goods[g.getId()].take(g);
    // Put good in equipment container.
    equipment.push_back(g);
}

void Traveler::equip(unsigned int pI) {
    // Equip fists if nothing is equipped in part.
    // Look for equipment in part.
    for (auto& e : equipment)
        for (auto& s : e.getMaterial().getCombatStats())
            if (s.partId == pI) return;
    if (pI == 2u) {
        // Add left fist to equipment.
        Good fist = Good(0u, "fist");
        Material fM(2u, "left");
        fM.setCombatStats({{1, 2, 1, 1, 0, {{1, 1, 1}}}, {2, 2, 0, 1, 1, {{1, 1, 1}}}});
        fist.addMaterial(fM);
        equipment.push_back(fist);
    } else if (pI == 3u) {
        // Add right fist to equipment.
        Good fist = Good(0u, "fist");
        Material fM(3u, "right");
        fM.setCombatStats({{1, 3, 1, 1, 0, {{1, 1, 1}}}, {2, 3, 0, 1, 1, {{1, 1, 1}}}});
        fist.addMaterial(fM);
        equipment.push_back(fist);
    }
}

void Traveler::refreshEquipButtons(std::vector<std::unique_ptr<TextBox>>& bs, const int& fB, size_t eBI, Printer& pr) {
    bs.erase(bs.begin() + static_cast<std::vector<std::unique_ptr<TextBox>>::difference_type>(eBI), bs.end());
    createEquipButtons(bs, fB, eBI, pr);
    if (fB > -1 && static_cast<size_t>(fB) < bs.size()) bs[static_cast<size_t>(fB)]->changeBorder(kBoxDBS);
}

void Traveler::createEquipButtons(std::vector<std::unique_ptr<TextBox>>& bs, const int& fB, size_t eBI, Printer& pr) {
    // Create buttons for equipping equippables.
    const SDL_Rect& sR = Settings::getScreenRect();
    const SDL_Color &fgr = nation->getForeground(), &bgr = nation->getBackground();
    int eB = Settings::getEquipBorder(), eR = Settings::getEquipRadius(), eFS = Settings::getEquipFontSize();
    int left = sR.w / kGoodButtonXDivisor, top = sR.h / kGoodButtonYDivisor,
        dx = sR.w * kGoodButtonSpaceMultiplier / kGoodButtonXDivisor, dy = sR.h * kGoodButtonSpaceMultiplier / kGoodButtonYDivisor;
    SDL_Rect rt = {left, top, sR.w * kGoodButtonSizeMultiplier / kGoodButtonXDivisor,
                   sR.h * kGoodButtonSizeMultiplier / kGoodButtonYDivisor};
    std::array<std::vector<Good>, 6> equippable; // array of vectors corresponding to parts that can hold
                                                 // equipment
    for (auto& g : goods)
        for (auto& m : g.getMaterials()) {
            auto& ss = m.getCombatStats();
            if (!ss.empty() && m.getAmount() >= 1) {
                // This good has combat stats and we have at least one of it.
                size_t i = ss.front().partId;
                Good e(g.getId(), g.getName(), 1);
                Material eM(m.getId(), m.getName(), 1, m.getImage());
                eM.setCombatStats(ss);
                e.addMaterial(eM);
                equippable[i].push_back(e);
            }
        }
    // Create buttons for equipping equipment.
    for (auto& eP : equippable) {
        for (auto& e : eP) {
            bs.push_back(e.button(false, rt, fgr, bgr, eB, eR, eFS, pr, [this, e, &bs, &fB, eBI, &pr]() mutable {
                equip(e);
                // Refresh buttons.
                refreshEquipButtons(bs, fB, eBI, pr);
            }));
            rt.y += dy;
        }
        rt.y = top;
        rt.x += dx;
    }
    // Create buttons for unequipping equipment.
    rt.y = top;
    left = sR.w * 218 / kGoodButtonXDivisor;
    for (auto& e : getEquipment()) {
        auto& ss = e.getMaterial().getCombatStats();
        unsigned int pI = ss.front().partId;
        rt.x = left + static_cast<int>(pI) * dx;
        bs.push_back(e.button(false, rt, fgr, bgr, eB, eR, eFS, pr, [this, pI, ss, &bs, &fB, eBI, &pr] {
            unequip(pI);
            // Equip fists.
            for (auto& s : ss) equip(s.partId);
            // Refresh buttons.
            refreshEquipButtons(bs, fB, eBI, pr);
        }));
    }
}

std::vector<std::shared_ptr<Traveler>> Traveler::attackable() const {
    // Get a vector of attackable travelers.
    auto& forwardTravelers = fromTown->getTravelers(); // travelers traveling from the same town
    auto& backwardTravelers = toTown->getTravelers();  // travelers traveling from the destination town
    std::vector<std::shared_ptr<Traveler>> able;
    able.insert(able.end(), forwardTravelers.begin(), forwardTravelers.end());
    if (fromTown != toTown) able.insert(able.end(), backwardTravelers.begin(), backwardTravelers.end());
    if (!able.empty()) {
        // Eliminate travelers which are too far away or have reached town or
        // are already fighting or are this traveler.
        able.erase(std::remove_if(able.begin(), able.end(),
                                  [this](std::shared_ptr<Traveler> t) {
                                      return t->toTown == t->fromTown || distSq(t->px, t->py) > Settings::getAttackDistSq() or
                                             t->target.lock() || !t->alive() || t == shared_from_this();
                                  }),
                   able.end());
    }
    return able;
}

void Traveler::attack(std::shared_ptr<Traveler> t) {
    // Attack the given parameter traveler.
    target = t;
    t->target = shared_from_this();
    fightTime = 0.;
    t->fightTime = 0.;
    if (ai)
        choice = FightChoice::fight;
    else
        choice = FightChoice::none;
    if (t->ai)
        t->ai->autoChoose();
    else
        t->choice = FightChoice::none;
}

void Traveler::createAttackButton(std::vector<std::unique_ptr<TextBox>>& bs, const std::function<void()>& sSF, Printer& pr) {
    // Put attackable traveler names in names vector.
    const SDL_Rect& sR = Settings::getScreenRect();
    int bBB = Settings::getBigBoxBorder(), bBR = Settings::getBigBoxRadius(), fFS = Settings::getFightFontSize();
    const SDL_Color &fgr = nation->getForeground(), &bgr = nation->getBackground(), &hl = nation->getHighlight();
    auto able = attackable();       // vector of attackable travelers
    std::vector<std::string> names; // vector of names of attackable travelers
    names.reserve(able.size());
    std::transform(able.begin(), able.end(), std::back_inserter(names),
                   [](const std::shared_ptr<Traveler> t) { return t->getName(); });
    // Create attack button.
    SDL_Rect rt = {sR.w / 4, sR.h / 4, sR.w / 2, sR.h / 2};
    bs.push_back(std::make_unique<SelectButton>(rt, names, fgr, bgr, hl, bBB, bBR, fFS, pr, [this, &bs, able, &sSF] {
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
    }
    target.reset();
}

CombatHit Traveler::firstHit(Traveler& t, std::uniform_real_distribution<>& d) {
    // Find first hit of this traveler on t.
    std::array<unsigned int, 3> defense{};
    for (auto& e : t.equipment)
        for (auto& s : e.getMaterial().getCombatStats())
            for (size_t i = 0; i < defense.size(); ++i) defense[i] += s.defense[i] * t.stats[s.statId];
    CombatHit first = {std::numeric_limits<double>::max(), 0u, 0u, ""};
    for (auto& e : equipment) {
        auto& ss = e.getMaterial().getCombatStats();
        if (ss.front().attack) {
            // e is a weapon
            unsigned int attack = 0u, type = ss.front().type, speed = 0u;
            for (auto& s : ss) {
                attack += s.attack * stats[s.statId];
                speed += s.speed * stats[s.statId];
            }
            auto cO = gameData.odds[type - 1];
            // Calculate number of swings before hit happens.
            double r = d(Settings::getRng());
            double p = static_cast<double>(attack) / cO.hitOdds / static_cast<double>(defense[type - 1]);
            double time;
            if (p < 1.)
                time = (log(r) / log(1. - p) + 1.) / speed;
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
                while (r > 0 && sCI != cO.statusChances.end()) {
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
    for (auto& e : equipment) {
        unsigned int sId = e.getShoots();
        if (sId) {
            unsigned int speed = 0;
            for (auto& s : e.getMaterial().getCombatStats()) speed += s.speed * stats[s.statId];
            unsigned int n = static_cast<unsigned int>(t * speed);
            goods[sId].use(n);
        }
    }
}

void Traveler::runFight(Traveler& t, unsigned int e, std::uniform_real_distribution<>& d) {
    // Fight t for e milliseconds.
    fightTime += e;
    // Prevent fight from happening twice.
    t.fightTime -= static_cast<double>(e);
    // Keep fighting until one side dies, runs, or yields or time runs out.
    while (alive() && t.alive() && choice == FightChoice::fight && t.choice == FightChoice::fight && fightTime > 0.) {
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
        if (ai) ai->autoChoose();
        if (t.ai) t.ai->autoChoose();
    }
}

void Traveler::takeHit(const CombatHit& cH, Traveler& t) {
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

void Traveler::createFightBoxes(std::vector<std::unique_ptr<TextBox>>& bs, bool& p, Printer& pr) {
    // Create buttons and text boxes for combat.
    const SDL_Rect& sR = Settings::getScreenRect();
    SDL_Rect rt = {sR.w / 2, sR.h / 4, 0, 0};
    auto tgt = target.lock();
    std::vector<std::string> tx = {"Fighting " + tgt->getName() + "..."};
    const SDL_Color &fgr = nation->getForeground(), &bgr = nation->getBackground(), &hl = nation->getHighlight();
    int b = Settings::getBigBoxBorder(), r = Settings::getBigBoxRadius(), fS = Settings::getFightFontSize();
    bs.push_back(std::make_unique<TextBox>(rt, tx, fgr, bgr, b, r, fS, pr));
    rt = {sR.w / 21, sR.h / 4, sR.w * 5 / 21, sR.h / 2};
    tx = {};
    bs.push_back(std::make_unique<TextBox>(rt, tx, fgr, bgr, b, r, fS, pr));
    rt = {sR.w * 15 / 21, sR.h / 4, sR.w * 5 / 21, sR.h / 2};
    tx = {};
    bs.push_back(std::make_unique<TextBox>(rt, tx, tgt->nation->getForeground(), tgt->nation->getBackground(), b, r, fS, pr));
    rt = {sR.w / 3, sR.h / 3, sR.w / 3, sR.h / 3};
    tx = {"Fight", "Run", "Yield"};
    bs.push_back(std::make_unique<SelectButton>(rt, tx, fgr, bgr, hl, b, r, fS, pr, [this, &bs, &p] {
        int choice = bs[kFightChoiceIndex]->getHighlightLine();
        if (choice > -1) {
            choose(static_cast<FightChoice>(choice));
            p = false;
        }
    }));
}

void Traveler::updateFightBoxes(std::vector<std::unique_ptr<TextBox>>& bs) {
    // Update TextBox objects for fight user interface.
    std::vector<std::string> choiceText = bs[0]->getText();
    switch (choice) {
    case FightChoice::none:
        break;
    case FightChoice::fight:
        if (choiceText[0] == "Running...") choiceText[0] = "Running... Caught!";
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
    if (!tgt) return;
    statusText[0] = tgt->getName() + "'s Status";
    for (size_t i = 1; i < statusText.size(); ++i)
        statusText[i] = gameData.parts[i - 1] + ": " + gameData.statuses[tgt->getPart(i - 1)];
    bs[2]->setText(statusText);
}

void Traveler::loot(Good& g, Traveler& t) {
    // Take the given good from the given traveler.
    unsigned int gId = g.getId();
    t.goods[gId].take(g);
    goods[gId].put(g);
}

void Traveler::loot(Traveler& t) {
    for (auto g : t.goods) loot(g, t);
}

void Traveler::refreshLootButtons(std::vector<std::unique_ptr<TextBox>>& bs, const int& fB, size_t lBI, Printer& pr) {
    bs.erase(bs.begin() + static_cast<std::vector<std::unique_ptr<TextBox>>::difference_type>(lBI), bs.end());
    createLootButtons(bs, fB, lBI, pr);
    if (fB > -1 && static_cast<size_t>(fB) < bs.size()) bs[static_cast<size_t>(fB)]->changeBorder(kBoxDBS);
}

void Traveler::createLootButtons(std::vector<std::unique_ptr<TextBox>>& bs, const int& fB, size_t lBI, Printer& pr) {
    const SDL_Rect& sR = Settings::getScreenRect();
    int tB = Settings::getTradeBorder(), tR = Settings::getTradeRadius(), tFS = Settings::getTradeFontSize();
    auto& tgt = *target.lock();
    const SDL_Color &fgr = nation->getForeground(), &bgr = nation->getBackground(), &tFgr = tgt.nation->getForeground(),
                    &tBgr = tgt.nation->getBackground();
    int left = sR.w / kGoodButtonXDivisor, right = sR.w / 2;
    int top = sR.h / kGoodButtonYDivisor;
    int dx = sR.w * kGoodButtonSpaceMultiplier / kGoodButtonXDivisor, dy = sR.h * kGoodButtonSpaceMultiplier / kGoodButtonYDivisor;
    SDL_Rect rt = {left, top, sR.w * kGoodButtonSizeMultiplier / kGoodButtonXDivisor,
                   sR.h * kGoodButtonSizeMultiplier / kGoodButtonYDivisor};
    for (auto& g : goods)
        for (auto& m : g.getMaterials()) {
            if ((m.getAmount() >= 0.01 && g.getSplit()) || (m.getAmount() >= 1)) {
                bs.push_back(g.button(true, m, rt, fgr, bgr, tB, tR, tFS, pr, [this, &g, &m, &tgt, &bs, &fB, lBI, &pr] {
                    Good lG(g.getId(), m.getAmount());
                    lG.addMaterial(Material(m.getId(), m.getAmount()));
                    tgt.loot(lG, *this);
                    refreshLootButtons(bs, fB, lBI, pr);
                }));
                rt.x += dx;
                if (rt.x + rt.w >= right) {
                    rt.x = left;
                    rt.y += dy;
                }
            }
        }
    left = sR.w / 2 + sR.w / kGoodButtonXDivisor;
    right = sR.w - sR.w / kGoodButtonXDivisor;
    rt.x = left;
    rt.y = top;
    for (auto& g : tgt.goods)
        for (auto& m : g.getMaterials()) {
            if ((m.getAmount() >= 0.01 && g.getSplit()) || (m.getAmount() >= 1)) {
                bs.push_back(g.button(true, m, rt, tFgr, tBgr, tB, tR, tFS, pr, [this, &g, &m, &tgt, &bs, &fB, lBI, &pr] {
                    Good lG(g.getId(), m.getAmount());
                    lG.addMaterial(Material(m.getId(), m.getAmount()));
                    loot(lG, tgt);
                    refreshLootButtons(bs, fB, lBI, pr);
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
    ai = std::make_unique<AI>(*this);
}

void Traveler::startAI(const Traveler& p) {
    // Starts an AI which imitates parameter's AI.
    ai = std::make_unique<AI>(*this, *p.ai);
}

void Traveler::runAI(unsigned int e) {
    // Run the AI for the elapsed time.
    ai->run(e);
}

void Traveler::update(unsigned int e) {
    // Move traveler toward destination and perform combat with target.
    if (toTown && moving) {
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
                              toTown->getNation()->getAdjective() + " " +
                              gameData.townTypeNouns[toTown->getTownType() - 1] + " of " + toTown->getName() + ".");
        }
    }
    if (auto t = target.lock()) {
        if (fightWon() && ai) {
            ai->autoLoot();
            loseTarget();
            return;
        }
        if (choice == FightChoice::fight) {
            static std::uniform_real_distribution<double> dis(0., 1.);
            double escapeChance;
            switch (t->choice) {
            case FightChoice::fight:
                runFight(*t, e, dis);
                break;
            case FightChoice::run:
                // Check if target escapes.
                escapeChance = Settings::getEscapeChance() * t->getSpeed() / getSpeed();
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

void Traveler::adjustAreas(const std::vector<std::unique_ptr<TextBox>>& bs, size_t i, double d) {
    std::vector<MenuButton*> requestButtons;
    for (auto rBI = bs.begin() + static_cast<std::vector<std::unique_ptr<TextBox>>::difference_type>(i); rBI != bs.end(); ++rBI)
        requestButtons.push_back(dynamic_cast<MenuButton*>(rBI->get()));
    toTown->adjustAreas(requestButtons, d);
}

void Traveler::adjustDemand(const std::vector<std::unique_ptr<TextBox>>& bs, size_t i, double d) {
    // Adjust demand for goods in current town and show new prices on buttons.
    std::vector<MenuButton*> requestButtons;
    for (auto rBI = bs.begin() + static_cast<std::vector<std::unique_ptr<TextBox>>::difference_type>(i); rBI != bs.end(); ++rBI)
        requestButtons.push_back(dynamic_cast<MenuButton*>(rBI->get()));
    toTown->adjustDemand(requestButtons, d);
}

void Traveler::resetTown() { toTown->resetGoods(); }

void Traveler::toggleMaxGoods() { toTown->toggleMaxGoods(); }

flatbuffers::Offset<Save::Traveler> Traveler::save(flatbuffers::FlatBufferBuilder& b) const {
    // Return a flatbuffers save object for this traveler.
    auto sName = b.CreateString(name);
    auto sLog = b.CreateVectorOfStrings(logText);
    auto sGoods =
        b.CreateVector<flatbuffers::Offset<Save::Good>>(goods.size(), [this, &b](size_t i) { return goods[i].save(b); });
    auto sProperties = b.CreateVector<flatbuffers::Offset<Save::Property>>(properties.size(), [this, &b](size_t i) {
        auto& property = properties[i];
        auto sStorage = b.CreateVector<flatbuffers::Offset<Save::Good>>(
            properties[i].storage.size(), [&property, &b](size_t j) { return property.storage[j].save(b); });
        auto sBusinesses = b.CreateVector<flatbuffers::Offset<Save::Business>>(
            properties[i].businesses.size(), [&property, &b](size_t j) { return property.businesses[j].save(b); });
        return Save::CreateProperty(b, sStorage, sBusinesses);
    });
    auto sStats = b.CreateVector(std::vector<unsigned int>(stats.begin(), stats.end()));
    auto sParts = b.CreateVector(std::vector<unsigned int>(parts.begin(), parts.end()));
    auto sEquipment = b.CreateVector<flatbuffers::Offset<Save::Good>>(
        equipment.size(), [this, &b](size_t i) { return equipment[i].save(b); });
    if (ai)
        return Save::CreateTraveler(b, sName, toTown->getId(), fromTown->getId(), nation->getId(), sLog, longitude,
                                    latitude, sGoods, sProperties, sStats, sParts, sEquipment, ai->save(b), moving);
    else
        return Save::CreateTraveler(b, sName, toTown->getId(), fromTown->getId(), nation->getId(), sLog, longitude,
                                    latitude, sGoods, sProperties, sStats, sParts, sEquipment, 0, moving);
}
