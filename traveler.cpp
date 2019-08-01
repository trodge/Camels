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
      moving(false), portion(1), properties(gD.townCount), gameData(gD) {
    // Copy goods vector from nation.
    const std::vector<Good> &gs = t->getNation()->getGoods();
    goods.reserve(gs.size());
    for (auto &g : gs) { goods.push_back(Good(g)); }
    // Create starting goods.
    goods[55].create(0.75);
    goods[59].create(2);
    // Equip fists.
    equip(2);
    equip(3);
    // Randomize stats.
    static std::uniform_int_distribution<unsigned int> dis(1, Settings::getStatMax());
    for (auto &s : stats) s = dis(Settings::getRng());
    // Set parts status to normal.
    parts.fill(0);
}

Traveler::Traveler(const Save::Traveler *t, std::vector<Town> &ts, const std::vector<Nation> &ns, const GameData &gD)
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
    auto &nGs = nation->getGoods();
    for (size_t i = 0; i < nGs.size(); ++i) {
        auto &nGMs = nGs[i].getMaterials();
        for (size_t j = 0; j < nGMs.size(); ++j) goods[i].setImage(j, nGMs[j].getImage());
    }
}

void Traveler::addToTown() { fromTown->addTraveler(this); }

double Traveler::netWeight() const {
    return std::accumulate(begin(goods), end(goods), static_cast<double>(stats[0]) * kTravelerCarry,
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
        if (c == d) return m->getId() < n->getId();
        return c < d;
    };
    // set of discovered towns not yet evaluated sorted so that shortest is first
    std::set<Town *, decltype(shortest)> open(shortest);
    open.insert(toTown);
    while (!open.empty()) {
        // find current town with lowest distance
        Town *current = *begin(open);
        const std::vector<Town *> &neighbors = current->getNeighbors();
        if (current == t) {
            while (from.count(current)) {
                path.push_front(current);
                current = from[current];
            }
            return path;
        }
        open.erase(begin(open));
        closed.insert(current);
        for (auto &n : neighbors) {
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

void Traveler::setPortion(double d) {
    portion = d;
    if (portion < 0)
        portion = 0;
    else if (portion > 1)
        portion = 1;
}

void Traveler::changePortion(double d) { setPortion(portion + d); }

void Traveler::pickTown(const Town *t) {
    if (netWeight() > 0 || moving) return;
    const std::forward_list<Town *> &path = pathTo(t);
    if (path.empty() || path.front() == toTown) return;
    moving = true;
    toTown = path.front();
}

void Traveler::place(int ox, int oy, double s) {
    px = static_cast<int>(s * longitude) + ox;
    py = oy - static_cast<int>(s * latitude);
}

void Traveler::draw(SDL_Renderer *s) const {
    SDL_Color col;
    if (aI)
        col = Settings::getAIColor();
    else
        col = Settings::getPlayerColor();
    drawCircle(s, px, py, 1, col, true);
}

void Traveler::updatePortionBox(TextBox *bx) const {
    // Give parameter box a string representing portion with trailing zeroes omitted.
    std::string portionString = std::to_string(portion);
    size_t lNZI = portionString.find_last_not_of('0');
    size_t dI = portionString.find('.');
    portionString.erase(lNZI == dI ? dI + 2 : lNZI + 1, std::string::npos);
    bx->setText(0, portionString);
}

void Traveler::clearTrade() {
    offer.clear();
    request.clear();
}

void Traveler::makeTrade() {
    if (!(offer.empty() || request.empty())) {
        std::string logEntry = name + " trades ";
        for (auto gI = begin(offer); gI != end(offer); ++gI) {
            if (gI != begin(offer)) {
                // This is not the first good in offer.
                if (offer.size() != 2) logEntry += ", ";
                if (gI + 1 == end(offer))
                    // This is the last good in offer.
                    logEntry += " and ";
            }
            goods[gI->getId()].take(*gI);
            toTown->put(*gI);
            logEntry += gI->logText();
        }
        logEntry += " for ";
        for (auto gI = begin(request); gI != end(request); ++gI) {
            if (gI != begin(request)) {
                // This is not the first good in request.
                if (request.size() != 2) logEntry += ", ";
                if (gI + 1 == end(request))
                    // This is the last good in request.
                    logEntry += " and ";
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
    for (auto &of : offer) {
        // Convert value to quantity of this good.
        auto &tM = toTown->getGood(of.getId()).getMaterial(of.getMaterial()); // town material
        double q = tM.getQuantity(ec / Settings::getTownProfit());
        if (!goods[of.getId()].getSplit()) q = floor(q);
        // Reduce quantity.
        of.use(q);
    }
}

void Traveler::createTradeButtons(std::vector<Pager> &pgrs, Printer &pr) {
    // Create offer and request buttons for the player on given pagers.
    const SDL_Rect &sR = Settings::getScreenRect();
    int gBXD = Settings::getGoodButtonXDivisor(), gBYD = Settings::getGoodButtonYDivisor(),
        gBSzM = Settings::getGoodButtonSizeMultiplier(), gBScM = Settings::getGoodButtonSpaceMultiplier();
    // Create the offer buttons for the player.
    SDL_Rect rt = {sR.w / 31, sR.h / 31, sR.w * 15 / 31, sR.h * 26 / 31};
    BoxInfo firstBox{.rect = {rt.x, rt.y, rt.w * gBSzM / gBXD, rt.h * gBSzM / gBYD},
                     .foreground = nation->getForeground(),
                     .background = nation->getBackground(),
                     .border = Settings::getTradeBorder(),
                     .radius = Settings::getTradeRadius(),
                     .fontSize = Settings::getTradeFontSize()};
    createGoodButtons(pgrs[1], goods, rt, rt.w * gBScM / gBXD, rt.h * gBScM / gBYD, firstBox, pr,
                      [this, &pgrs](const Good &, const Material &) {
                          return [this, &pgrs](MenuButton *) { updateTradeButtons(pgrs); };
                      });
    rt.x = sR.w * 16 / 31;
    firstBox.rect.x = rt.x;
    firstBox.foreground = toTown->getNation()->getForeground();
    firstBox.background = toTown->getNation()->getBackground();
    createGoodButtons(pgrs[2], toTown->getGoods(), rt, rt.w * gBScM / gBXD, rt.h * gBScM / gBYD, firstBox, pr,
                      [this, &pgrs](const Good &, const Material &) {
                          return [this, &pgrs](MenuButton *) { updateTradeButtons(pgrs); };
                      });
}

void Traveler::updateTradeButtons(std::vector<Pager> &pgrs) {
    // Update the values shown on offer and request boxes and set offer and request.
    std::string bN;
    clearTrade();
    double offerValue = 0.;
    // Loop through all offer buttons.
    std::vector<TextBox *> bxs = pgrs[1].getAll();
    for (auto bx : bxs) {
        auto &g = goods[bx->getId()]; // good corresponding to bIt
        bN = bx->getText()[0];        // name of material and good on button
        auto &m = g.getMaterial(bN);
        m.updateButton(g.getSplit(), bx);
        if (bx->getClicked()) {
            // Button was clicked, so add corresponding good to offer.
            double amount = m.getAmount() * portion;
            if (!g.getSplit()) amount = floor(amount);
            auto &tM = toTown->getGood(g.getId()).getMaterial(m); // town material
            double p = tM.getPrice(amount);
            if (p > 0.) {
                offerValue += p;
                offer.push_back(Good(g.getId(), g.getName(), amount, g.getMeasure()));
                offer.back().addMaterial(Material(m.getId(), m.getName(), amount));
            } else
                // Good is worthless in this town, don't allow it to be offered.
                bx->setClicked(false);
        }
    }
    bxs = pgrs[2].getAll();
    unsigned int requestCount =
        std::accumulate(begin(bxs), end(bxs), 0, [](unsigned int c, const TextBox *rB) { return c + rB->getClicked(); }); // count of clicked request buttons.
    request.reserve(requestCount);
    double excess = 0.; // excess value of offer over value needed for request
    // Loop through request buttons.
    for (auto bx : bxs) {
        auto &g = toTown->getGood(bx->getId()); // good in town corresponding to bIt
        bN = bx->getText()[0];
        auto &m = g.getMaterial(bN);
        if (offer.size()) {
            m.updateButton(g.getSplit(), offerValue, std::max(1u, requestCount), bx);
            if (bx->getClicked() && offerValue > 0.) {
                double mE = 0; // excess quantity of this material
                double amount = m.getQuantity(offerValue / requestCount * Settings::getTownProfit(), mE);
                if (!g.getSplit())
                    // Remove extra portion of goods that don't split.
                    mE += modf(amount, &amount);
                // Convert material excess to value and add to overall excess.
                excess += m.getPrice(mE);
                request.push_back(Good(g.getId(), g.getName(), amount, g.getMeasure()));
                request.back().addMaterial(Material(m.getId(), m.getName(), amount));
            }
        } else
            m.updateButton(g.getSplit(), bx);
    }
    if (offer.size()) divideExcess(excess);
}

void Traveler::deposit(Good &g) {
    // Put the given good in storage in the current town.
    auto &storage = properties[toTown->getId() - 1].storage;
    if (storage.empty()) {
        // Storage has not been created yet.
        auto &townGoods = toTown->getGoods(); // goods from the town
        storage.reserve(townGoods.size());
        for (auto &g : townGoods) {
            // Copy goods from town, omitting amounts.
            storage.push_back(Good(g.getId(), g.getName(), g.getPerish(), g.getCarry(), g.getMeasure()));
            for (auto &m : g.getMaterials()) storage.back().addMaterial(Material(m.getId(), m.getName(), m.getImage()));
        }
    }
    unsigned int gId = g.getId();
    goods[gId].take(g);
    storage[gId].put(g);
}

void Traveler::withdraw(Good &g) {
    // Take the given good from storage in the current town.
    auto &storage = properties[toTown->getId() - 1].storage;
    if (storage.empty())
        // Cannot withdraw from empty storage.
        return;
    unsigned int gId = g.getId();
    storage[gId].take(g);
    goods[gId].put(g);
}

void Traveler::refreshFocusBox(std::vector<Pager> &pgrs, int &fB) {
    // Run toggle focus on the focused box if it was just recreated.
    if (fB < 0) return;
    int focusBox = fB;
    for (auto pgrIt = begin(pgrs) + 1; pgrIt != end(pgrs); ++pgrIt) {
        int visibleCount = pgrIt->visibleCount();
        if (focusBox < visibleCount) {
            pgrIt->getVisible(focusBox)->toggleFocus();
            break;
        } else
            focusBox -= visibleCount;
    }
}

void Traveler::refreshStorageButtons(std::vector<Pager> &pgrs, int &fB, Printer &pr) {
    // Delete and re-create storage buttons to reflect changes.
    for (auto pgrIt = begin(pgrs) + 1; pgrIt != end(pgrs); ++pgrIt) pgrIt->reset();
    createStorageButtons(pgrs, fB, pr);
    refreshFocusBox(pgrs, fB);
}

void Traveler::createStorageButtons(std::vector<Pager> &pgrs, int &fB, Printer &pr) {
    // Create buttons for depositing and withdrawing goods to and from the current town.
    const SDL_Rect &sR = Settings::getScreenRect();
    int gBXD = Settings::getGoodButtonXDivisor(), gBYD = Settings::getGoodButtonYDivisor(),
        gBSzM = Settings::getGoodButtonSizeMultiplier(), gBScM = Settings::getGoodButtonSpaceMultiplier();
    // Create the offer buttons for the player.
    SDL_Rect rt = {sR.w / 31, sR.h / 31, sR.w * 15 / 31, sR.h * 26 / 31};
    BoxInfo firstBox{.rect = {rt.x, rt.y, rt.w * gBSzM / gBXD, rt.h * gBSzM / gBYD},
                     .foreground = nation->getForeground(),
                     .background = nation->getBackground(),
                     .border = Settings::getTradeBorder(),
                     .radius = Settings::getTradeRadius(),
                     .fontSize = Settings::getTradeFontSize()};
    createGoodButtons(pgrs[1], goods, rt, rt.w * gBScM / gBXD, rt.h * gBScM / gBYD, firstBox, pr,
                      [this, &pgrs, &fB, &pr](const Good &g, const Material &m) {
                          return [this, &g, &m, &pgrs, &fB, &pr](MenuButton *) {
                              Good dG(g.getId(), g.getAmount() * portion);
                              dG.addMaterial(Material(m.getId(), m.getAmount()));
                              deposit(dG);
                              refreshStorageButtons(pgrs, fB, pr);
                          };
                      });
    rt.x = sR.w * 16 / 31;
    firstBox.rect.x = rt.x;
    firstBox.foreground = toTown->getNation()->getForeground();
    firstBox.background = toTown->getNation()->getBackground();
    createGoodButtons(pgrs[2], toTown->getGoods(), rt, rt.w * gBScM / gBXD, rt.h * gBScM / gBYD, firstBox, pr,
                      [this, &pgrs, &fB, &pr](const Good &g, const Material &m) {
                          return [this, &g, &m, &pgrs, &fB, &pr](MenuButton *) {
                              Good dG(g.getId(), g.getAmount() * portion);
                              dG.addMaterial(Material(m.getId(), m.getAmount()));
                              withdraw(dG);
                              refreshStorageButtons(pgrs, fB, pr);
                          };
                      });
}

void Traveler::build(const Business &bsn, double a) {
    // Build the given area of the given business. Check requirements before
    // calling. Determine if business exists in property.
    auto &businesses = properties[toTown->getId() - 1].businesses;
    auto bsnsIt = std::lower_bound(begin(businesses), end(businesses), bsn);
    if (bsnsIt == end(businesses) || *bsnsIt != bsn) {
        // Insert new business into properties and set its area.
        auto nwBsns = businesses.insert(bsnsIt, Business(bsn));
        nwBsns->takeRequirements(goods, a);
        nwBsns->setArea(a);
    } else {
        // Business already exists, grow it.
        bsnsIt->takeRequirements(goods, a);
        bsnsIt->changeArea(a);
    }
}

void Traveler::demolish(const Business &bsn, double a) {
    // Demolish the given area of the given business. Check area before calling.
    auto &oBsns = properties[toTown->getId() - 1].businesses;
    auto oBsnIt = std::lower_bound(begin(oBsns), end(oBsns), bsn);
    oBsnIt->changeArea(-a);
    oBsnIt->reclaim(goods, a);
    if (oBsnIt->getArea() == 0.) oBsns.erase(oBsnIt);
}

void Traveler::refreshManageButtons(std::vector<Pager> &pgrs, int &fB, Printer &pr) {
    // Delete and re-create manage buttons to reflect changes.
    for (auto pgrIt = begin(pgrs) + 1; pgrIt != end(pgrs); ++pgrIt) pgrIt->reset();
    createManageButtons(pgrs, fB, pr);
    refreshFocusBox(pgrs, fB);
}

void Traveler::createManageButtons(std::vector<Pager> &pgrs, int &fB, Printer &pr) {
    // Create buttons for managing businesses.
    const SDL_Rect &sR = Settings::getScreenRect();
    const SDL_Color &fgr = nation->getForeground(), &bgr = nation->getBackground(),
                    &tFgr = toTown->getNation()->getForeground(), &tBgr = toTown->getNation()->getBackground();
    int tB = Settings::getTradeBorder(), tR = Settings::getTradeRadius(), tFS = Settings::getTradeFontSize(),
        bBXD = Settings::getBusinessButtonXDivisor(), bBYD = Settings::getBusinessButtonYDivisor(),
        bBSzM = Settings::getBusinessButtonSizeMultiplier(), bBScM = Settings::getBusinessButtonSpaceMultiplier();
    // Create buttons for demolishing businesses.
    std::vector<Business> &oBsns = properties[toTown->getId() - 1].businesses;
    int left = sR.w / bBXD, right = sR.w / 2, top = sR.h / bBYD, bottom = sR.h * 13 / 14,
        dx = (right - left) * bBScM / bBXD, dy = (bottom - top) * bBScM / bBYD;
    BoxInfo boxInfo{.rect = {left, top, (right - left) * bBSzM / bBXD, (bottom - top) * bBSzM / bBYD},
                    .foreground = fgr,
                    .background = bgr,
                    .border = tB,
                    .radius = tR,
                    .fontSize = tFS};
    std::vector<std::unique_ptr<TextBox>> bxs;
    for (auto &bsn : oBsns) {
        boxInfo.onClick = [this, &bsn, &pgrs, &fB, &pr](MenuButton *) {
            demolish(bsn, bsn.getArea() * portion);
            refreshManageButtons(pgrs, fB, pr);
        };
        bxs.push_back(bsn.button(true, boxInfo, pr));
        boxInfo.rect.x += dx;
        if (boxInfo.rect.x + boxInfo.rect.w >= right) {
            // Reached end of line, carriage return.
            boxInfo.rect.x = left;
            boxInfo.rect.y += dy;
            if (boxInfo.rect.y + boxInfo.rect.h >= bottom) {
                // Reached bottom of page, flush vector.
                pgrs[1].addPage(bxs);
                boxInfo.rect.y = top;
            }
        }
    }
    if (bxs.size())
        // Flush remaining boxes to new page.
        pgrs[1].addPage(bxs);
    const std::vector<Business> &tBsns = toTown->getBusinesses();
    left = sR.w / 2 + sR.w / bBXD;
    right = sR.w - sR.w / bBXD;
    boxInfo.rect.x = left;
    boxInfo.rect.y = top;
    boxInfo.foreground = tFgr;
    boxInfo.background = tBgr;
    for (auto &bsn : tBsns) {
        boxInfo.onClick = [this, &bsn, &pgrs, &fB, &pr](MenuButton *) {
            // Determine buildable area based on portion.
            double buildable = std::numeric_limits<double>::max();
            for (auto &rq : bsn.getRequirements())
                buildable = std::min(buildable, goods[rq.getId()].getAmount() * portion / rq.getAmount());
            if (buildable > 0.) build(bsn, buildable);
            refreshManageButtons(pgrs, fB, pr);
        };
        bxs.push_back(bsn.button(true, boxInfo, pr));
        boxInfo.rect.x += dx;
        if (boxInfo.rect.x + boxInfo.rect.w >= right) {
            // Reached end of line, carriage return.
            boxInfo.rect.x = left;
            boxInfo.rect.y += dy;
            if (boxInfo.rect.y + boxInfo.rect.h >= bottom) {
                // Reached bottom of page, flush vector.
                pgrs[2].addPage(bxs);
                boxInfo.rect.y = top;
            }
        }
    }
    if (bxs.size())
        // Flush remaining boxes to new page.
        pgrs[2].addPage(bxs);
}

void Traveler::unequip(unsigned int pI) {
    // Unequip all equipment using the given part id.
    auto unused = [pI](const Good &e) {
        auto &eSs = e.getMaterial().getCombatStats();
        return std::find_if(begin(eSs), end(eSs), [pI](const CombatStat &s) { return s.partId == pI; }) == end(eSs);
    };
    auto uEI = std::partition(begin(equipment), end(equipment), unused);
    // Put equipment back in goods.
    for (auto eI = uEI; eI != end(equipment); ++eI)
        if (eI->getAmount() > 0.) goods[eI->getId()].put(*eI);
    equipment.erase(uEI, end(equipment));
}

void Traveler::equip(Good &g) {
    // Equip the given good.
    auto &ss = g.getMaterial().getCombatStats();
    std::vector<unsigned int> pIs; // part ids used by this equipment
    pIs.reserve(ss.size());
    for (auto &s : ss)
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
    for (auto &e : equipment)
        for (auto &s : e.getMaterial().getCombatStats())
            if (s.partId == pI) return;
    if (pI == 2) {
        // Add left fist to equipment.
        Good fist = Good(0, "fist");
        Material fM(2, "left");
        fM.setCombatStats({{1, 2, 1, 1, 0, {{1, 1, 1}}}, {2, 2, 0, 1, 1, {{1, 1, 1}}}});
        fist.addMaterial(fM);
        equipment.push_back(fist);
    } else if (pI == 3) {
        // Add right fist to equipment.
        Good fist = Good(0, "fist");
        Material fM(3, "right");
        fM.setCombatStats({{1, 3, 1, 1, 0, {{1, 1, 1}}}, {2, 3, 0, 1, 1, {{1, 1, 1}}}});
        fist.addMaterial(fM);
        equipment.push_back(fist);
    }
}

void Traveler::refreshEquipButtons(std::vector<Pager> &pgrs, int &fB, Printer &pr) {
    // Delete and re-create storage buttons to reflect changes.
    for (auto pgrIt = begin(pgrs) + 1; pgrIt != end(pgrs); ++pgrIt) pgrIt->reset();
    createEquipButtons(pgrs, fB, pr);
    refreshFocusBox(pgrs, fB);
}

void Traveler::createEquipButtons(std::vector<Pager> &pgrs, int &fB, Printer &pr) {
    // Create buttons for equipping equippables.
    const SDL_Rect &sR = Settings::getScreenRect();
    const SDL_Color &fgr = nation->getForeground(), &bgr = nation->getBackground();
    int eB = Settings::getEquipBorder(), eR = Settings::getEquipRadius(), eFS = Settings::getEquipFontSize(),
        gBXD = Settings::getGoodButtonXDivisor(), gBYD = Settings::getGoodButtonYDivisor(),
        gBSzM = Settings::getGoodButtonSizeMultiplier(), gBScM = Settings::getGoodButtonSpaceMultiplier();
    int left = sR.w / gBXD, top = sR.h / gBYD, dx = sR.w * gBScM / gBXD / 2, dy = sR.h * gBScM / gBYD;
    BoxInfo boxInfo{.rect = {left, top, sR.w * gBSzM / gBXD / 2, sR.h * gBSzM / gBYD / 2},
                    .foreground = fgr,
                    .background = bgr,
                    .border = eB,
                    .radius = eR,
                    .fontSize = eFS};
    std::array<std::vector<Good>, 6> equippable; // array of vectors corresponding to parts that can hold
                                                 // equipment
    for (auto &g : goods)
        for (auto &m : g.getMaterials()) {
            auto &ss = m.getCombatStats();
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
    for (auto &eP : equippable) {
        for (auto &e : eP) {
            boxInfo.onClick = [this, e, &pgrs, &fB, &pr](MenuButton *) mutable {
                equip(e);
                // Refresh buttons.
                refreshEquipButtons(pgrs, fB, pr);
            };
            pgrs[1].addBox(e.button(false, boxInfo, pr));
            boxInfo.rect.y += dy;
        }
        boxInfo.rect.y = top;
        boxInfo.rect.x += dx;
    }
    // Create buttons for unequipping equipment.
    boxInfo.rect.y = top;
    left = sR.w / 2 + sR.w / gBXD;
    for (auto &e : equipment) {
        auto &ss = e.getMaterial().getCombatStats();
        unsigned int pI = ss.front().partId;
        boxInfo.rect.x = left + static_cast<int>(pI) * dx;
        boxInfo.onClick = [this, e, &pgrs, pI, &ss, &fB, &pr](MenuButton *) {
            unequip(pI);
            // Equip fists.
            for (auto &s : ss) equip(s.partId);
            // Refresh buttons.
            refreshEquipButtons(pgrs, fB, pr);
        };
        pgrs[1].addBox(e.button(false, boxInfo, pr));
    }
}

std::vector<Traveler *> Traveler::attackable() const {
    // Get a vector of attackable travelers.
    auto &forwardTravelers = fromTown->getTravelers(); // travelers traveling from the same town
    auto &backwardTravelers = toTown->getTravelers();  // travelers traveling from the destination town
    std::vector<Traveler *> able;
    able.insert(end(able), begin(forwardTravelers), end(forwardTravelers));
    if (fromTown != toTown) able.insert(end(able), begin(backwardTravelers), end(backwardTravelers));
    if (!able.empty()) {
        // Eliminate travelers which are too far away or have reached town or
        // are already fighting or are this traveler.
        able.erase(std::remove_if(begin(able), end(able),
                                  [this](Traveler *t) {
                                      return t->toTown == t->fromTown || distSq(t->px, t->py) > Settings::getAttackDistSq() ||
                                             t->target || !t->alive() || t == this;
                                  }),
                   end(able));
    }
    return able;
}

void Traveler::attack(Traveler *t) {
    // Attack the given parameter traveler.
    target = t;
    t->target = this;
    fightTime = 0.;
    t->fightTime = 0.;
    if (aI)
        choice = FightChoice::fight;
    else
        choice = FightChoice::none;
    if (t->aI)
        t->aI->autoChoose();
    else
        t->choice = FightChoice::none;
}

void Traveler::createAttackButton(Pager &pgr, std::function<void()> sSF, Printer &pr) {
    // Put attackable traveler names in names vector.
    const SDL_Rect &sR = Settings::getScreenRect();
    int bBB = Settings::getBigBoxBorder(), bBR = Settings::getBigBoxRadius(), fFS = Settings::getFightFontSize();
    const SDL_Color &fgr = nation->getForeground(), &bgr = nation->getBackground(), &hl = nation->getHighlight();
    auto able = attackable();       // vector of attackable travelers
    std::vector<std::string> names; // vector of names of attackable travelers
    names.reserve(able.size());
    std::transform(begin(able), end(able), std::back_inserter(names), [](const Traveler *t) { return t->getName(); });
    // Create attack button.
    pgr.addBox(std::make_unique<SelectButton>(BoxInfo{{sR.w / 4, sR.h / 4, sR.w / 2, sR.h / 2},
                                                      names,
                                                      fgr,
                                                      bgr,
                                                      hl,
                                                      0,
                                                      false,
                                                      bBB,
                                                      bBR,
                                                      fFS,
                                                      {},
                                                      SDLK_f,
                                                      [this, able, sSF](MenuButton *btn) {
                                                          int i = btn->getHighlightLine();
                                                          if (i > -1) {
                                                              attack(able[static_cast<size_t>(i)]);
                                                              sSF();
                                                          } else
                                                              btn->setClicked(false);
                                                      },
                                                      true},
                                              pr));
}

void Traveler::loseTarget() {
    if (auto t = target) {
        t->target = nullptr;
        // Remove target from town so that it can't be attacked again.
        t->fromTown->removeTraveler(t);
        // Set dead to true if target is not alive.
        t->dead = !t->alive();
    }
    target = nullptr;
}

CombatHit Traveler::firstHit(Traveler &t, std::uniform_real_distribution<> &d) {
    // Find first hit of this traveler on t.
    std::array<unsigned int, 3> defense{};
    for (auto &e : t.equipment)
        for (auto &s : e.getMaterial().getCombatStats())
            for (size_t i = 0; i < defense.size(); ++i) defense[i] += s.defense[i] * t.stats[s.statId];
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
                auto sCI = begin(cO.statusChances);
                while (r > 0 && sCI != end(cO.statusChances)) {
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
            for (auto &s : e.getMaterial().getCombatStats()) speed += s.speed * stats[s.statId];
            unsigned int n = static_cast<unsigned int>(t * speed);
            goods[sId].use(n);
        }
    }
}

void Traveler::runFight(Traveler &t, unsigned int e, std::uniform_real_distribution<> &d) {
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
        if (aI) aI->autoChoose();
        if (t.aI) t.aI->autoChoose();
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

void Traveler::createFightBoxes(Pager &pgr, bool &p, Printer &pr) {
    // Create buttons and text boxes for combat.
    const SDL_Rect &sR = Settings::getScreenRect();
    auto tgt = target;
    const SDL_Color &fgr = nation->getForeground(), &bgr = nation->getBackground(), &hgl = nation->getHighlight(),
                    &tFgr = tgt->nation->getForeground(), &tBgr = tgt->nation->getBackground(),
                    &tHgl = tgt->nation->getHighlight();
    int b = Settings::getBigBoxBorder(), r = Settings::getBigBoxRadius(), fS = Settings::getFightFontSize();
    auto ourBox = [&fgr, &bgr, &hgl, b, r, fS](const SDL_Rect &rt, const std::vector<std::string> &tx) {
        return BoxInfo{rt, tx, fgr, bgr, hgl, 0, false, b, r, fS, {}};
    };
    auto theirBox = [&tFgr, &tBgr, &tHgl, b, r, fS](const SDL_Rect &rt, const std::vector<std::string> &tx) {
        return BoxInfo{rt, tx, tFgr, tBgr, tHgl, 0, false, b, r, fS, {}};
    };
    auto selectButton = [&fgr, &bgr, &hgl, b, r, fS](const SDL_Rect &rt, const std::vector<std::string> &tx,
                                                     SDL_Keycode ky, std::function<void(MenuButton *)> fn) {
        return BoxInfo{rt, tx, fgr, bgr, hgl, 0, false, b, r, fS, {}, ky, fn, true};
    };
    pgr.addBox(std::make_unique<TextBox>(ourBox({sR.w / 2, sR.h / 4, 0, 0}, {"Fighting " + tgt->getName() + "..."}), pr));
    pgr.addBox(std::make_unique<TextBox>(ourBox({sR.w / 21, sR.h / 4, sR.w * 5 / 21, sR.h / 2}, {}), pr));
    pgr.addBox(std::make_unique<TextBox>(theirBox({sR.w * 15 / 21, sR.h / 4, sR.w * 5 / 21, sR.h / 2}, {}), pr));
    pgr.addBox(std::make_unique<SelectButton>(
        selectButton({sR.w / 3, sR.h / 3, sR.w / 3, sR.h / 3}, {"Fight", "Run", "Yield"}, SDLK_c,
                     [this, &p](MenuButton *btn) {
                         int choice = btn->getHighlightLine();
                         if (choice > -1) {
                             choose(static_cast<FightChoice>(choice));
                             p = false;
                         }
                     }),
        pr));
}

void Traveler::updateFightBoxes(Pager &pgr) {
    // Update TextBox objects for fight user interface.
    std::vector<TextBox *> bxs = pgr.getVisible();
    std::vector<std::string> choiceText = bxs[0]->getText();
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
    bxs[0]->setText(choiceText);
    std::vector<std::string> statusText(7);
    statusText[0] = name + "'s Status";
    for (size_t i = 1; i < statusText.size(); ++i)
        statusText[i] = gameData.parts[i - 1] + ": " + gameData.statuses[getPart(i - 1)];
    bxs[1]->setText(statusText);
    auto tgt = target;
    if (!tgt) return;
    statusText[0] = tgt->getName() + "'s Status";
    for (size_t i = 1; i < statusText.size(); ++i)
        statusText[i] = gameData.parts[i - 1] + ": " + gameData.statuses[tgt->getPart(i - 1)];
    bxs[2]->setText(statusText);
}

void Traveler::loot(Good &g, Traveler &t) {
    // Take the given good from the given traveler.
    unsigned int gId = g.getId();
    t.goods[gId].take(g);
    goods[gId].put(g);
}

void Traveler::loot(Traveler &t) {
    for (auto g : t.goods) loot(g, t);
}

void Traveler::refreshLootButtons(std::vector<Pager> &pgrs, int &fB, Printer &pr) {
    for (auto pgrIt = begin(pgrs) + 1; pgrIt != end(pgrs); ++pgrIt) pgrIt->reset();
    createLootButtons(pgrs, fB, pr);
    refreshFocusBox(pgrs, fB);
}

void Traveler::createLootButtons(std::vector<Pager> &pgrs, int &fB, Printer &pr) {
    const SDL_Rect &sR = Settings::getScreenRect();
    int tB = Settings::getTradeBorder(), tR = Settings::getTradeRadius(), tFS = Settings::getTradeFontSize(),
        gBXD = Settings::getGoodButtonXDivisor(), gBYD = Settings::getGoodButtonYDivisor(),
        gBSzM = Settings::getGoodButtonSizeMultiplier(), gBScM = Settings::getGoodButtonSpaceMultiplier();
    auto &tgt = *target;
    const SDL_Color &fgr = nation->getForeground(), &bgr = nation->getBackground(), &tFgr = tgt.nation->getForeground(),
                    &tBgr = tgt.nation->getBackground();
    int left = sR.w / gBXD, right = sR.w / 2, top = sR.h / gBYD, bottom = sR.h * 13 / 14,
        dx = (right - left) * gBScM / gBXD, dy = (bottom - top) * gBScM / gBYD;
    BoxInfo boxInfo{.rect = {left, top, (right - left) * gBSzM / gBXD, (bottom - top) * gBSzM / gBYD},
                    .foreground = fgr,
                    .background = bgr,
                    .border = tB,
                    .radius = tR,
                    .fontSize = tFS};
    std::vector<std::unique_ptr<TextBox>> bxs;
    for (auto &g : goods)
        for (auto &m : g.getMaterials()) {
            if ((m.getAmount() >= 0.01 && g.getSplit()) || (m.getAmount() >= 1)) {
                boxInfo.onClick = [this, &g, &m, &tgt, &pgrs, &fB, &pr](MenuButton *) {
                    Good lG(g.getId(), m.getAmount());
                    lG.addMaterial(Material(m.getId(), m.getAmount()));
                    tgt.loot(lG, *this);
                    refreshLootButtons(pgrs, fB, pr);
                };
                bxs.push_back(g.button(true, m, boxInfo, pr));
                boxInfo.rect.x += dx;
                if (boxInfo.rect.x + boxInfo.rect.w >= right) {
                    boxInfo.rect.x = left;
                    boxInfo.rect.y += dy;
                    if (boxInfo.rect.y + boxInfo.rect.h >= bottom) {
                        // Flush current page upon reaching bottom.
                        boxInfo.rect.y = top;
                        pgrs[1].addPage(bxs);
                    }
                }
            }
        }

    if (bxs.size())
        // Flush remaining boxes to new page.
        pgrs[1].addPage(bxs);
    left = sR.w / 2 + sR.w / gBXD;
    right = sR.w - sR.w / gBXD;
    boxInfo.rect.x = left;
    boxInfo.rect.y = top;
    boxInfo.foreground = tFgr;
    boxInfo.background = tBgr;
    for (auto &g : tgt.goods)
        for (auto &m : g.getMaterials()) {
            if ((m.getAmount() >= 0.01 && g.getSplit()) || (m.getAmount() >= 1)) {
                boxInfo.onClick = [this, &g, &m, &tgt, &pgrs, &fB, &pr](MenuButton *) {
                    Good lG(g.getId(), m.getAmount());
                    lG.addMaterial(Material(m.getId(), m.getAmount()));
                    loot(lG, tgt);
                    refreshLootButtons(pgrs, fB, pr);
                };
                bxs.push_back(g.button(true, m, boxInfo, pr));
                boxInfo.rect.x += dx;
                if (boxInfo.rect.x + boxInfo.rect.w >= right) {
                    boxInfo.rect.x = left;
                    boxInfo.rect.y += dy;
                    if (boxInfo.rect.y + boxInfo.rect.h >= bottom) {
                        // Flush current page upon reaching bottom.
                        boxInfo.rect.y = top;
                        pgrs[2].addPage(bxs);
                    }
                }
            }
        }
    if (bxs.size())
        // Flush remaining boxes to new page.
        pgrs[2].addPage(bxs);
}

void Traveler::startAI() {
    // Initialized variables for running a new AI.
    aI = std::make_unique<AI>(*this);
}

void Traveler::startAI(const Traveler &p) {
    // Starts an AI which imitates parameter's AI.
    aI = std::make_unique<AI>(*this, *p.aI);
}

void Traveler::runAI(unsigned int e) {
    // Run the AI for the elapsed time.
    aI->run(e);
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
            fromTown->removeTraveler(this);
            toTown->addTraveler(this);
            fromTown = toTown;
            moving = false;
            logText.push_back(name + " has arrived in the " +
                              gameData.populationAdjectives.lower_bound(toTown->getPopulation())->second + " " +
                              toTown->getNation()->getAdjective() + " " +
                              gameData.townTypeNouns[toTown->getTownType() - 1] + " of " + toTown->getName() + ".");
        }
    }
    if (auto t = target) {
        if (fightWon() && aI) {
            aI->autoLoot();
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

void Traveler::adjustAreas(Pager &pgr, double mM) {
    std::vector<MenuButton *> requestButtons;
    std::vector<TextBox *> bxs = pgr.getAll();
    for (auto rBI = begin(bxs); rBI != end(bxs); ++rBI) requestButtons.push_back(dynamic_cast<MenuButton *>(*rBI));
    toTown->adjustAreas(requestButtons, mM);
}

void Traveler::adjustDemand(Pager &pgr, double mM) {
    // Adjust demand for goods in current town and show new prices on buttons.
    std::vector<MenuButton *> requestButtons;
    std::vector<TextBox *> bxs = pgr.getAll();
    for (auto rBI = begin(bxs); rBI != end(bxs); ++rBI) requestButtons.push_back(dynamic_cast<MenuButton *>(*rBI));
    toTown->adjustDemand(requestButtons, mM);
}

void Traveler::resetTown() { toTown->resetGoods(); }

void Traveler::toggleMaxGoods() { toTown->toggleMaxGoods(); }

flatbuffers::Offset<Save::Traveler> Traveler::save(flatbuffers::FlatBufferBuilder &b) const {
    // Return a flatbuffers save object for this traveler.
    auto sName = b.CreateString(name);
    auto sLog = b.CreateVectorOfStrings(logText);
    auto sGoods =
        b.CreateVector<flatbuffers::Offset<Save::Good>>(goods.size(), [this, &b](size_t i) { return goods[i].save(b); });
    auto sProperties = b.CreateVector<flatbuffers::Offset<Save::Property>>(properties.size(), [this, &b](size_t i) {
        auto &property = properties[i];
        auto sStorage = b.CreateVector<flatbuffers::Offset<Save::Good>>(
            properties[i].storage.size(), [&property, &b](size_t j) { return property.storage[j].save(b); });
        auto sBusinesses = b.CreateVector<flatbuffers::Offset<Save::Business>>(
            properties[i].businesses.size(), [&property, &b](size_t j) { return property.businesses[j].save(b); });
        return Save::CreateProperty(b, sStorage, sBusinesses);
    });
    auto sStats = b.CreateVector(std::vector<unsigned int>(begin(stats), end(stats)));
    auto sParts = b.CreateVector(std::vector<unsigned int>(begin(parts), end(parts)));
    auto sEquipment = b.CreateVector<flatbuffers::Offset<Save::Good>>(
        equipment.size(), [this, &b](size_t i) { return equipment[i].save(b); });
    if (aI)
        return Save::CreateTraveler(b, sName, toTown->getId(), fromTown->getId(), nation->getId(), sLog, longitude,
                                    latitude, sGoods, sProperties, sStats, sParts, sEquipment, aI->save(b), moving);
    else
        return Save::CreateTraveler(b, sName, toTown->getId(), fromTown->getId(), nation->getId(), sLog, longitude,
                                    latitude, sGoods, sProperties, sStats, sParts, sEquipment, 0, moving);
}
