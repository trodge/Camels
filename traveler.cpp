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

Traveler::Traveler(const std::string &n, Town *tn, const GameData &gD)
    : name(n), nation(tn->getNation()), toTown(tn), fromTown(tn), position(tn->getPosition()), moving(false),
      portion(1), reputation(gD.nationCount), gameData(gD) {
    // Copy goods vector from nation.
    properties.emplace(std::piecewise_construct, std::forward_as_tuple(0),
                       std::forward_as_tuple(false, &tn->getNation()->getProperty()));
    // Equip fists.
    equip(Part::leftArm);
    equip(Part::rightArm);
    // Randomize stats.
    stats = Settings::travelerStats();
    // Set parts status to normal.
    parts.fill(Status::normal);
}

Traveler::Traveler(const Save::Traveler *ldTvl, const std::vector<Nation> &nts, std::vector<Town> &tns, const GameData &gD)
    : name(ldTvl->name()->str()), nation(&nts[static_cast<size_t>(ldTvl->nation() - 1)]),
      toTown(&tns[static_cast<size_t>(ldTvl->toTown() - 1)]),
      fromTown(&tns[static_cast<size_t>(ldTvl->fromTown() - 1)]),
      position(ldTvl->longitude(), ldTvl->latitude()), moving(ldTvl->moving()), portion(1), gameData(gD) {
    auto ldLog = ldTvl->log();
    std::transform(ldLog->begin(), ldLog->end(), std::back_inserter(logText),
                   [](auto ldL) { return ldL->str(); });
    // Copy good image pointers from nation's goods to traveler's goods.
    auto ntPpt = &nation->getProperty();
    auto ldProperties = ldTvl->properties();
    for (auto ldPI = ldProperties->begin(); ldPI != ldProperties->end(); ++ldPI)
        properties.emplace(std::piecewise_construct, std::forward_as_tuple((*ldPI)->townId()),
                           std::forward_as_tuple(*ldPI, ntPpt));
    auto ldStats = ldTvl->stats();
    std::transform(ldStats->begin(), ldStats->end(), begin(stats), [](auto ldSt) { return ldSt; });
    auto ldParts = ldTvl->parts();
    std::transform(ldParts->begin(), ldParts->end(), begin(parts),
                   [](auto ldPt) { return static_cast<Status>(ldPt); });
    auto ldEquipment = ldTvl->equipment();
    std::transform(ldEquipment->begin(), ldEquipment->end(), std::back_inserter(equipment),
                   [](auto ldEq) { return Good(ldEq); });
}

flatbuffers::Offset<Save::Traveler> Traveler::save(flatbuffers::FlatBufferBuilder &b) const {
    // Return a flatbuffers save object for this traveler.
    auto svName = b.CreateString(name);
    auto svLog = b.CreateVectorOfStrings(logText);
    std::vector<std::pair<unsigned int, Property>> vPpts(begin(properties), end(properties));
    auto svProperties = b.CreateVector<flatbuffers::Offset<Save::Property>>(
        properties.size(), [&b, &vPpts](size_t i) { return vPpts[i].second.save(b, vPpts[i].first); });
    auto svStats = b.CreateVector(std::vector<unsigned int>(begin(stats), end(stats)));
    std::vector<unsigned int> vParts(static_cast<size_t>(Part::count));
    std::transform(begin(parts), end(parts), begin(vParts),
                   [](auto pt) { return static_cast<unsigned int>(pt); });
    auto svParts = b.CreateVector(vParts);
    auto svEquipment = b.CreateVector<flatbuffers::Offset<Save::Good>>(
        equipment.size(), [this, &b](size_t i) { return equipment[i].save(b); });
    if (aI)
        return Save::CreateTraveler(b, svName, toTown->getId(), fromTown->getId(), nation->getId(), svLog,
                                    position.getLongitude(), position.getLatitude(), svProperties, svStats,
                                    svParts, svEquipment, aI->save(b), moving);
    else
        return Save::CreateTraveler(b, svName, toTown->getId(), fromTown->getId(), nation->getId(), svLog,
                                    position.getLongitude(), position.getLatitude(), svProperties, svStats,
                                    svParts, svEquipment, 0, moving);
}

double Traveler::weight() const {
    return properties.find(0)->second.weight() + static_cast<double>(stats[0]) * kTravelerCarry;
}

std::forward_list<Town *> Traveler::pathTo(const Town *tn) const {
    // Return forward list of towns on shortest path to tn, excluding current town.
    std::forward_list<Town *> path;
    std::unordered_map<Town *, Town *> from;
    // the town which each town can be most efficiently reached from
    std::unordered_map<Town *, int> distSqTo({{toTown, 0}});
    // distance along route to each town
    std::unordered_map<Town *, int> distSqEst({{toTown, toTown->distSq(tn)}});
    // estimated distance along route through each town to goal
    std::unordered_set<Town *> closed;
    // set of towns already evaluated
    auto shortest = [&distSqEst](Town *m, Town *n) {
        int c = distSqEst[m], d = distSqEst[n];
        if (c == d) return m->getId() < n->getId();
        return c < d;
    };
    std::set<Town *, decltype(shortest)> open(shortest);
    // set of discovered towns not yet evaluated sorted so that closest is first
    open.insert(toTown);
    while (!open.empty()) {
        // Find current town with lowest distance.
        Town *current = *begin(open);
        const std::vector<Town *> &neighbors = current->getNeighbors();
        if (current == tn) {
            // Current town is target town.
            while (from.count(current)) {
                path.push_front(current);
                current = from[current];
            }
            return path;
        }
        open.erase(begin(open));
        closed.insert(current);
        for (auto &nb : neighbors) {
            // Ignore town if already explored.
            if (closed.count(nb)) continue;
            // Find distance to nb through current.
            int dSqT = distSqTo[current] + current->distSq(nb);
            // Check if distance through current is less than previous shortest path to nb.
            if (!distSqTo.count(nb) || dSqT < distSqTo[nb]) {
                from[nb] = current;
                distSqTo[nb] = dSqT;
                distSqEst[nb] = dSqT + nb->distSq(tn);
            }
            // Add undiscovered node to open set.
            if (!open.count(nb)) open.insert(nb);
        }
    }
    // A path does not exist, return empty list.
    return path;
}

int Traveler::pathDistSq(const Town *tn) const {
    // Return the distance to tn along shortest path.
    const std::forward_list<Town *> &path = pathTo(tn);
    int distSq = 0;
    const Town *m = toTown;
    for (auto &n : path) {
        distSq += m->distSq(n);
        m = n;
    }
    return distSq;
}

void Traveler::addToTown() { toTown->addTraveler(this); }

void Traveler::setPortion(double d) {
    portion = d;
    if (portion < 0)
        portion = 0;
    else if (portion > 1)
        portion = 1;
}

void Traveler::changePortion(double d) { setPortion(portion + d); }

void Traveler::create(unsigned int gId, double amt) { properties.find(0)->second.create(gId, amt); }

void Traveler::pickTown(const Town *tn) {
    // Start moving toward given town.
    if (weight() > 0 || moving) return;
    const auto &path = pathTo(tn);
    if (path.empty() || path.front() == toTown) return;
    toTown = path.front();
    moving = true;
}

void Traveler::draw(SDL_Renderer *s) const {
    SDL_Color col;
    if (aI)
        col = Settings::getAIColor();
    else
        col = Settings::getPlayerColor();
    drawCircle(s, position.getPoint(), 1, col, true);
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
    if (offer.empty() || request.empty()) return;
    auto &ppt = properties.find(0)->second;
    std::string logEntry = name + " trades ";
    for (auto gI = begin(offer); gI != end(offer); ++gI) {
        if (gI != begin(offer)) {
            // This is not the first good in offer.
            if (offer.size() != 2) logEntry += ", ";
            if (gI + 1 == end(offer))
                // This is the last good in offer.
                logEntry += " and ";
        }
        ppt.take(*gI);
        toTown->put(*gI);
        logEntry += gI->logEntry();
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
        ppt.put(*gI);
        logEntry += gI->logEntry();
    }
    logEntry += " in " + toTown->getName() + ".";
    logText.push_back(logEntry);
}

void Traveler::divideExcess(double exc, double tnP) {
    // Divide excess value among amounts of offered goods.
    exc /= static_cast<double>(offer.size());
    for (auto &of : offer) {
        // Convert value to quantity of this good.
        auto tG = toTown->getProperty().good(of.getFullId()); // pointer to town good
        double q = tG->quantity(exc / tnP);
        if (!tG->getSplit()) q = floor(q);
        // Reduce quantity.
        of.use(q);
    }
}

void Traveler::createTradeButtons(std::vector<Pager> &pgrs, Printer &pr) {
    // Create offer and request buttons for the player on given pagers.
    const SDL_Rect &sR = Settings::getScreenRect();
    int m = Settings::getButtonMargin();
    // Create the offer buttons for the player.
    SDL_Rect rt = {m, sR.h * 2 / 31, sR.w * 15 / 31, sR.h * 25 / 31};
    BoxInfo bxInf = boxInfo();
    properties.find(0)->second.buttons(pgrs[1], rt, bxInf, pr, [this, &pgrs](const Good &) {
        return [this, &pgrs](MenuButton *) { updateTradeButtons(pgrs); };
    });
    rt.x = sR.w - m - rt.w;
    bxInf.colors = toTown->getNation()->getColors();
    toTown->buttons(pgrs[2], rt, bxInf, pr, [this, &pgrs](const Good &) {
        return [this, &pgrs](MenuButton *) { updateTradeButtons(pgrs); };
    });
}

void Traveler::updateTradeButtons(std::vector<Pager> &pgrs) {
    // Update the values shown on offer and request boxes and set offer and request.
    std::string bN;
    clearTrade();
    double offerValue = 0;
    // Loop through all offer buttons.
    std::vector<TextBox *> bxs(pgrs[1].getAll());
    for (auto bx : bxs)
        if (bx->getClicked()) {
            auto gd = properties.find(0)->second.good(bx->getId()); // pointer to good corresponding to box
            // Button was clicked, so add corresponding good to offer.
            double amount = gd->getAmount() * portion;
            if (!gd->getSplit()) amount = floor(amount);
            auto tG = toTown->getProperty().good(gd->getFullId()); // pointer to town good
            double p = tG->price(amount);
            if (p > 0) {
                offerValue += p;
                offer.push_back(Good(gd->getFullId(), gd->getFullName(), amount, gd->getMeasure()));
            } else
                // Good is worthless in this town, don'tn allow it to be offered.
                bx->setClicked(false);
        }
    double townProfit = Settings::getTownProfit();
    offerValue *= townProfit;
    bxs = pgrs[2].getAll();
    unsigned int requestCount = std::accumulate(begin(bxs), end(bxs), 0, [](unsigned int c, const TextBox *rB) {
        return c + rB->getClicked();
    }); // count of clicked request buttons.
    request.reserve(requestCount);
    double excess = 0; // excess value of offer over value needed for request
    // Loop through request buttons.
    for (auto bx : bxs) {
        auto tG = toTown->getProperty().good(bx->getId()); // pointer to good in town corresponding to box
        if (offer.empty())
            tG->updateButton(bx);
        else {
            tG->updateButton(offerValue, std::max(1u, requestCount), bx);
            if (bx->getClicked() && offerValue > 0) {
                double mE = 0; // excess quantity of this material
                double amount = tG->quantity(offerValue / requestCount, mE);
                if (!tG->getSplit())
                    // Remove extra portion of goods that don'tn split.
                    mE += modf(amount, &amount);
                // Convert material excess to value and add to overall excess.
                excess += tG->price(mE);
                request.push_back(Good(tG->getFullId(), tG->getFullName(), amount, tG->getMeasure()));
            }
        }
    }
    if (offer.size()) divideExcess(excess, townProfit);
}

Property &Traveler::makeProperty(unsigned int tId) {
    // Returns a reference to property in the given town id, or carried property if 0.
    auto pptIt = properties.find(tId);
    if (pptIt == end(properties))
        // Property has not been created yet.
        pptIt = properties
                    .emplace(std::piecewise_construct, std::forward_as_tuple(tId),
                             std::forward_as_tuple(toTown->getProperty().getCoastal(), &toTown->getNation()->getProperty()))
                    .first;
    return pptIt->second;
}

void Traveler::deposit(Good &g) {
    // Put the given good in storage in the current town.
    properties.find(0)->second.take(g);
    makeProperty(toTown->getId()).put(g);
}

void Traveler::withdraw(Good &g) {
    // Take the given good from storage in the current town.
    makeProperty(toTown->getId()).take(g);
    properties.find(0)->second.put(g);
}

void Traveler::refreshFocusBox(std::vector<Pager> &pgrs, int &fB) {
    // Run toggle focus on the focused box if it was just recreated.
    if (fB < 0) return;
    int focusBox = fB - pgrs[0].visibleCount();
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
    int m = Settings::getButtonMargin();
    // Create the offer buttons for the player.
    SDL_Rect rt{m, sR.h * 2 / 31, sR.w * 15 / 31, sR.h * 25 / 31};
    BoxInfo bxInf = boxInfo();
    properties.find(0)->second.buttons(pgrs[1], rt, bxInf, pr, [this, &pgrs, &fB, &pr](const Good &g) {
        return [this, &g, &pgrs, &fB, &pr](MenuButton *) {
            double amt = g.getAmount() * portion;
            Good dG(g.getFullId(), amt);
            deposit(dG);
            refreshStorageButtons(pgrs, fB, pr);
        };
    });
    rt.x = sR.w - m - rt.w;
    bxInf.colors = toTown->getNation()->getColors();
    property(toTown->getId()).buttons(pgrs[2], rt, bxInf, pr, [this, &pgrs, &fB, &pr](const Good &g) {
        return [this, &g, &pgrs, &fB, &pr](MenuButton *) {
            double amt = g.getAmount() * portion;
            Good wG(g.getFullId(), amt);
            withdraw(wG);
            refreshStorageButtons(pgrs, fB, pr);
        };
    });
}

void Traveler::build(const Business &bsn, double a) {
    // Build the given area of the given business. Check requirements before calling.
    makeProperty(toTown->getId()).build(bsn, a);
}

void Traveler::demolish(const Business &bsn, double a) {
    // Demolish the given area of the given business. Check area before calling.
    makeProperty(toTown->getId()).demolish(bsn, a);
}

void Traveler::refreshBuildButtons(std::vector<Pager> &pgrs, int &fB, Printer &pr) {
    // Delete and re-create manage buttons to reflect changes.
    for (auto pgrIt = begin(pgrs) + 1; pgrIt != end(pgrs); ++pgrIt) pgrIt->reset();
    createBuildButtons(pgrs, fB, pr);
    refreshFocusBox(pgrs, fB);
}

void Traveler::createBuildButtons(std::vector<Pager> &pgrs, int &fB, Printer &pr) {
    // Create buttons for managing businesses.
    const SDL_Rect &sR = Settings::getScreenRect();
    // Create buttons for demolishing businesses.
    int m = Settings::getButtonMargin();
    SDL_Rect rt = {m, sR.h * 2 / 31, sR.w * 15 / 31, sR.h * 25 / 31};
    pgrs[1].setBounds(rt);
    BoxInfo bxInf = boxInfo();
    property(toTown->getId()).buttons(pgrs[1], rt, bxInf, pr, [this, &pgrs, &fB, &pr](const Business &bsn) {
        return [this, &bsn, &pgrs, &fB, &pr](MenuButton *) {
            demolish(bsn, bsn.getArea() * portion);
            refreshBuildButtons(pgrs, fB, pr);
        };
    });
    rt.x = sR.w - m - rt.w;
    bxInf.rect.x = rt.x;
    bxInf.rect.y = rt.y;
    bxInf.colors = toTown->getNation()->getColors();
    toTown->getProperty().buttons(pgrs[2], rt, bxInf, pr, [this, &pgrs, &fB, &pr](const Business &bsn) {
        return [this, &bsn, &pgrs, &fB, &pr](MenuButton *) {
            // Determine buildable area based on portion.
            double buildable = std::numeric_limits<double>::max();
            for (auto &rq : bsn.getRequirements())
                buildable = std::min(
                    buildable, properties.find(0)->second.amount(rq.getGoodId()) * portion / rq.getAmount());
            if (buildable > 0) build(bsn, buildable);
            refreshBuildButtons(pgrs, fB, pr);
        };
    });
}

void Traveler::unequip(Part pt) {
    // Unequip all equipment using the given part id.
    auto unused = [pt](const Good &e) {
        auto &eSs = e.getCombatStats();
        return std::find_if(begin(eSs), end(eSs), [pt](const CombatStat &s) { return s.part == pt; }) == end(eSs);
    };
    auto uEI = std::partition(begin(equipment), end(equipment), unused);
    // Put equipment back in goods.
    for (auto eI = uEI; eI != end(equipment); ++eI)
        if (eI->getAmount() > 0) properties.find(0)->second.put(*eI);
    equipment.erase(uEI, end(equipment));
}

void Traveler::equip(Good &g) {
    // Equip the given good.
    auto &ss = g.getCombatStats();
    std::vector<Part> pts; // parts used by this equipment
    pts.reserve(ss.size());
    for (auto &s : ss)
        if (pts.empty() || pts.back() != s.part) pts.push_back(s.part);
    for (auto pt : pts) unequip(pt);
    // Take good out of goods.
    properties.find(0)->second.take(g);
    // Put good in equipment container.
    equipment.push_back(g);
}

void Traveler::equip(Part pt) {
    // Equip fists if nothing is equipped in part.
    // Look for equipment in part.
    for (auto &e : equipment)
        for (auto &s : e.getCombatStats())
            if (s.part == pt) return;
    if (pt == Part::leftArm) {
        // Add left fist to equipment.
        Good fist = Good(0, "left fist", 1,
                         {{Part::leftArm, Stat::strength, 1, 0, AttackType::bash, {{1, 1, 1}}},
                          {Part::leftArm, Stat::agility, 0, 1, AttackType::bash, {{1, 1, 1}}}},
                         nullptr);
        equipment.push_back(fist);
    } else if (pt == Part::rightArm) {
        // Add right fist to equipment.
        Good fist = Good(0, "right fist", 1,
                         {{Part::rightArm, Stat::strength, 1, 0, AttackType::bash, {{1, 1, 1}}},
                          {Part::rightArm, Stat::agility, 0, 1, AttackType::bash, {{1, 1, 1}}}},
                         nullptr);
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
    int m = Settings::getButtonMargin();
    SDL_Rect rt{m, sR.h * 2 / 31, sR.w * 15 / 31, sR.h * 26 / 31};
    pgrs[1].setBounds(rt);
    int dx = (rt.w + m) / static_cast<int>(Part::count), dy = (rt.h + m) / Settings::getGoodButtonRows();
    BoxInfo bxInf = boxInfo({rt.x, rt.y, dx - m, dy - m}, {}, BoxSizeType::equip);
    std::array<std::vector<Good>, static_cast<size_t>(Part::count)> equippable;
    // array of vectors corresponding to parts that can hold equipment
    properties.find(0)->second.forGood([&equippable](const Good &g) {
        auto &ss = g.getCombatStats();
        if (!ss.empty() && g.getAmount() >= 1) {
            // This good has combat stats and we have at least one of it.
            Part pt = ss.front().part;
            Good e(g.getFullId(), g.getFullName(), 1, ss, g.getImage());
            equippable[static_cast<size_t>(pt)].push_back(e);
        }
    });
    // Create buttons for equipping equipment.
    for (auto &eP : equippable) {
        for (auto &e : eP) {
            bxInf.onClick = [this, e, &pgrs, &fB, &pr](MenuButton *) mutable {
                equip(e);
                // Refresh buttons.
                refreshEquipButtons(pgrs, fB, pr);
            };
            pgrs[1].addBox(e.button(false, bxInf, pr));
            bxInf.rect.y += dy;
        }
        bxInf.rect.y = rt.y;
        bxInf.rect.x += dx;
    }
    // Create buttons for unequipping equipment.
    rt.x = sR.w - m - rt.w;
    pgrs[2].setBounds(rt);
    bxInf.rect.x = rt.x;
    bxInf.rect.y = rt.y;
    for (auto &e : equipment) {
        auto &ss = e.getCombatStats();
        Part pt = ss.front().part;
        bxInf.rect.x = rt.x + static_cast<int>(pt) * dx;
        bxInf.onClick = [this, e, &pgrs, pt, &ss, &fB, &pr](MenuButton *) {
            unequip(pt);
            // Equip fists.
            for (auto &s : ss) equip(s.part);
            // Refresh buttons.
            refreshEquipButtons(pgrs, fB, pr);
        };
        pgrs[1].addBox(e.button(false, bxInf, pr));
    }
}

void Traveler::createManageButtons(std::vector<Pager> &pgrs, int &fB, Printer &pr) {
    // Create buttons for hiring and managing employees.
    const SDL_Rect &sR = Settings::getScreenRect();
    int m = Settings::getButtonMargin();
    // Create the offer buttons for the player.
    SDL_Rect rt{m, sR.h * 2 / 31, sR.w * 15 / 31, sR.h * 25 / 31};
    BoxInfo bxInf = boxInfo();
    properties.find(0)->second.buttons(pgrs[1], rt, bxInf, pr, [](const Good &) { return [](MenuButton *) {}; });
    auto &bids = toTown->getBids();
    std::vector<std::string> names;
    for (auto &bd : bids) names.push_back(bd->party->name);
    pgrs[2].addBox(std::make_unique<SelectButton>(
        Settings::boxInfo({sR.w * 17 / 31, sR.h / 31, sR.w * 12 / 31, sR.h * 11 / 31}, names,
                          toTown->getNation()->getColors(), BoxSizeType::big, SDLK_h,
                          [](MenuButton *) {

                          }),
        pr));
}

std::vector<Traveler *> Traveler::attackable() const {
    // Get a vector of attackable travelers.
    auto &forwardTravelers = fromTown->getTravelers(); // travelers traveling from the same town
    auto &backwardTravelers = toTown->getTravelers();  // travelers traveling from the destination town
    std::vector<Traveler *> able;
    able.insert(end(able), begin(forwardTravelers), end(forwardTravelers));
    if (fromTown != toTown) able.insert(end(able), begin(backwardTravelers), end(backwardTravelers));
    if (!able.empty()) {
        // Eliminate travelers which are too far away or have reached town or are already fighting or are this traveler.
        int aDstSq = Settings::getAttackDistSq();
        able.erase(std::remove_if(begin(able), end(able),
                                  [this, aDstSq](Traveler *tg) {
                                      return !tg->moving || position.distSq(tg->position) > aDstSq ||
                                             tg->target || !tg->alive() || tg == this;
                                  }),
                   end(able));
    }
    return able;
}

void Traveler::attack(Traveler *tgt) {
    // Attack the given parameter traveler.
    target = tgt;
    tgt->target = this;
    fightTime = 0;
    tgt->fightTime = 0;
    if (aI)
        choice = FightChoice::fight;
    else
        choice = FightChoice::none;
    if (tgt->aI)
        tgt->aI->choose();
    else
        tgt->choice = FightChoice::none;
}

void Traveler::createAttackButton(Pager &pgr, std::function<void()> sSF, Printer &pr) {
    // Put attackable traveler names in names vector.
    const SDL_Rect &sR = Settings::getScreenRect();
    auto able = attackable();       // vector of attackable travelers
    std::vector<std::string> names; // vector of names of attackable travelers
    names.reserve(able.size());
    std::transform(begin(able), end(able), std::back_inserter(names),
                   [](const Traveler *tg) { return tg->getName(); });
    // Create attack button.
    pgr.addBox(std::make_unique<SelectButton>(
        boxInfo({sR.w / 4, sR.h / 4, sR.w / 2, sR.h / 2}, names, BoxSizeType::fight, BoxBehavior::scroll, SDLK_f,
                [this, able, sSF](MenuButton *btn) {
                    int i = btn->getHighlightLine();
                    if (i > -1) {
                        attack(able[static_cast<size_t>(i)]);
                        sSF();
                    } else
                        btn->setClicked(false);
                }),
        pr));
}

void Traveler::createLogBox(Pager &pgr, Printer &pr) {
    // Create log box.
    const SDL_Rect &sR = Settings::getScreenRect();
    pgr.addBox(std::make_unique<ScrollBox>(
        boxInfo({sR.w / 15, sR.h * 2 / 15, sR.w * 28 / 31, sR.h * 11 / 15}, logText, BoxSizeType::small), pr));
}

void Traveler::loseTarget() {
    if (target) {
        target->target = nullptr;
        // Remove target from town so that it can'tn be attacked again.
        target->fromTown->removeTraveler(target);
        // Set dead to true if target is not alive.
        target->dead = !target->alive();
        target = nullptr;
    }
}

CombatHit Traveler::firstHit(Traveler &tgt) {
    // Find first hit of this traveler on tn.
    std::array<unsigned int, 3> defense{};
    for (auto &e : tgt.equipment)
        for (auto &s : e.getCombatStats())
            for (size_t i = 0; i < defense.size(); ++i)
                defense[i] += s.defense[i] * tgt.stats[static_cast<size_t>(s.stat)];
    CombatHit first = {std::numeric_limits<double>::max(), nullptr, ""};
    for (auto &e : equipment) {
        auto &ss = e.getCombatStats();
        if (ss.front().attack) {
            // e is a weapon.
            unsigned int attack = 0, speed = 0;
            AttackType type = ss.front().type;
            for (auto &s : ss) {
                attack += s.attack * stats[static_cast<size_t>(s.stat)];
                speed += s.speed * stats[static_cast<size_t>(s.stat)];
            }
            auto &cO = gameData.odds[static_cast<size_t>(type)];
            // Calculate number of swings before hit happens.
            double r = Settings::random();
            double p = static_cast<double>(attack) / cO.hitChance /
                       static_cast<double>(defense[static_cast<size_t>(type)]);
            double time;
            if (p < 1)
                time = (log(r) / log(1 - p) + 1) / speed;
            else
                time = 1 / speed;
            if (time < first.time) {
                first.time = time;
                first.odd = &cO;
                first.weapon = e.getFullName();
            }
        }
    }
    return first;
}

void Traveler::useAmmo(double tn) {
    for (auto &e : equipment) {
        unsigned int sId = e.getShoots();
        if (sId) {
            unsigned int speed = 0;
            for (auto &s : e.getCombatStats()) speed += s.speed * stats[static_cast<size_t>(s.stat)];
            unsigned int n = static_cast<unsigned int>(tn * speed);
            properties.find(0)->second.use(sId, n);
        }
    }
}

void Traveler::fight(Traveler &tgt, unsigned int elTm) {
    // Fight tgt for e milliseconds.
    fightTime += elTm;
    // Prevent fight from happening twice.
    tgt.fightTime -= static_cast<double>(elTm);
    // Keep fighting until one side dies, runs, or yields or time runs out.
    while (alive() && tgt.alive() && choice == FightChoice::fight && tgt.choice == FightChoice::fight && fightTime > 0) {
        CombatHit ourFirst = firstHit(tgt), theirFirst = tgt.firstHit(*this);
        if (ourFirst.time < theirFirst.time) {
            // Our hit happens first.
            tgt.takeHit(ourFirst, *this);
            fightTime -= ourFirst.time;
            useAmmo(ourFirst.time);
            tgt.useAmmo(ourFirst.time);
        } else if (theirFirst.time < ourFirst.time) {
            // Their hit happens first.
            takeHit(theirFirst, tgt);
            fightTime -= theirFirst.time;
            useAmmo(theirFirst.time);
            tgt.useAmmo(theirFirst.time);
        } else {
            // Both hits happen at the same time.
            takeHit(theirFirst, tgt);
            tgt.takeHit(ourFirst, *this);
            fightTime -= ourFirst.time;
            useAmmo(ourFirst.time);
            tgt.useAmmo(ourFirst.time);
        }
        if (aI) aI->choose();
        if (tgt.aI) tgt.aI->choose();
    }
}

void Traveler::takeHit(const CombatHit &cH, Traveler &tgt) {
    // Apply the given combat hit from the given traveler to this traveler and update both logs.
    // Pick a random part.
    auto part = Settings::part();
    // Start status at that part's status.
    auto status = parts[static_cast<size_t>(part)];
    // Get random value from 0 to 1.
    auto r = Settings::random();
    // Find status such that part becomes more damaged.
    if (!cH.odd) return;
    auto &stChcs = cH.odd->statusChances;
    for (auto sCI = begin(stChcs); r > 0 && sCI != end(stChcs); ++sCI)
        // Find status such that part becomes more damaged.
        if (sCI->first > status) {
            // This part is less damaged than the current status.
            status = sCI->first;
            // Subtract chance of this status from r.
            r -= sCI->second;
        }
    parts[static_cast<size_t>(part)] = status;
    if (status > Status::wounded)
        // Part is too wounded to hold equipment.
        unequip(part);
    std::string logEntry = tgt.name + "'s " + cH.weapon + " strikes " + name + ". " + name + "'s " +
                           gameData.partNames[static_cast<size_t>(part)] + " has been " +
                           gameData.statusNames[static_cast<size_t>(status)] + ".";
    logText.push_back(logEntry);
    tgt.logText.push_back(logEntry);
}

void Traveler::createFightBoxes(Pager &pgr, bool &p, Printer &pr) {
    // Create buttons and text boxes for combat.
    const SDL_Rect &sR = Settings::getScreenRect();
    pgr.addBox(std::make_unique<TextBox>(
        boxInfo({sR.w / 2, sR.h / 4, 0, 0}, {"Fighting " + target->getName() + "..."}, BoxSizeType::fight), pr));
    pgr.addBox(std::make_unique<TextBox>(
        boxInfo({sR.w / 21, sR.h / 4, sR.w * 5 / 21, sR.h / 2}, {}, BoxSizeType::fight), pr));
    pgr.addBox(std::make_unique<TextBox>(
        target->boxInfo({sR.w * 15 / 21, sR.h / 4, sR.w * 5 / 21, sR.h / 2}, {}, BoxSizeType::fight), pr));
    pgr.addBox(std::make_unique<SelectButton>(boxInfo({sR.w / 3, sR.h / 3, sR.w / 3, sR.h / 3},
                                                      {"Fight", "Run", "Yield"}, BoxSizeType::fight, BoxBehavior::scroll, SDLK_c,
                                                      [this, &p](MenuButton *btn) {
                                                          int hl = btn->getHighlightLine();
                                                          if (hl > -1) {
                                                              choice = static_cast<FightChoice>(hl);
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
        if (choiceText[0] == "Running...") choiceText[0] += " Caught!";
        break;
    case FightChoice::run:
        choiceText[0] = "Running...";
        break;
    case FightChoice::yield:
        choiceText[0] = "Yielding...";
        break;
    }
    bxs[0]->setText(choiceText);
    std::vector<std::string> statusText(static_cast<size_t>(Part::count) + 1);
    statusText[0] = name + "'s Status";
    for (size_t i = 1; i < static_cast<size_t>(Part::count) + 1; ++i)
        statusText[i] = gameData.partNames[i - 1] + ": " +
                        gameData.statusNames[static_cast<size_t>(part(static_cast<Part>(i - 1)))];
    bxs[1]->setText(statusText);
    auto tgt = target;
    if (!tgt) return;
    statusText[0] = tgt->getName() + "'s Status";
    for (size_t i = 1; i < statusText.size(); ++i)
        statusText[i] = gameData.partNames[i - 1] + ": " +
                        gameData.statusNames[static_cast<size_t>(part(static_cast<Part>(i - 1)))];
    bxs[2]->setText(statusText);
}

void Traveler::loot(Good &g) {
    // Take the given good from the given traveler.
    target->properties.find(0)->second.take(g);
    properties.find(0)->second.put(g);
}

void Traveler::loot() {
    target->property().forGood([this](const Good &g) {
        Good lG(g);
        loot(lG);
    });
}

void Traveler::refreshLootButtons(std::vector<Pager> &pgrs, int &fB, Printer &pr) {
    for (auto pgrIt = begin(pgrs) + 1; pgrIt != end(pgrs); ++pgrIt) pgrIt->reset();
    createLootButtons(pgrs, fB, pr);
    refreshFocusBox(pgrs, fB);
}

void Traveler::createLootButtons(std::vector<Pager> &pgrs, int &fB, Printer &pr) {
    const SDL_Rect &sR = Settings::getScreenRect();
    int m = Settings::getButtonMargin();
    SDL_Rect rt{m, sR.h * 2 / 31, sR.w * 15 / 31, sR.h * 25 / 31};
    BoxInfo bxInf = boxInfo();
    // Create buttons for leaving goods behind.
    properties.find(0)->second.buttons(pgrs[1], rt, bxInf, pr, [this, &pgrs, &fB, &pr](const Good &g) {
        return [this, &g, &pgrs, &fB, &pr](MenuButton *) {
            Good lG(g.getFullId(), g.getAmount() * portion);
            target->loot(lG);
            refreshLootButtons(pgrs, fB, pr);
        };
    });
    rt.x = sR.w - m - rt.w;
    bxInf.colors = target->nation->getColors();
    // Create buttons for looting goods.
    target->properties.find(0)->second.buttons(pgrs[2], rt, bxInf, pr, [this, &pgrs, &fB, &pr](const Good &g) {
        return [this, &g, &pgrs, &fB, &pr](MenuButton *) {
            Good lG(g.getFullId(), g.getAmount() * portion);
            loot(lG);
            refreshLootButtons(pgrs, fB, pr);
        };
    });
}

void Traveler::createAIGoods() {
    for (auto &sG : Settings::getAIStartingGoods(aI->getRole())) create(sG.first, sG.second);
}

void Traveler::startAI() {
    // Initialize variables for running a new AI.
    aI = std::make_unique<AI>(*this);
    createAIGoods();
}

void Traveler::startAI(const Traveler &p) {
    // Starts an AI which imitates parameter's AI.
    aI = std::make_unique<AI>(*this, *p.aI);
    createAIGoods();
}

void Traveler::update(unsigned int elTm) {
    // Move traveler toward destination, update properties, and perform combat with target.
    if (aI) aI->update(elTm);
    for (auto &ppt : properties) ppt.second.update(elTm);
    if (moving) {
        moving = !position.stepToward(
            toTown->getPosition(), static_cast<double>(elTm) / static_cast<double>(Settings::getDayLength()));
        if (!moving) {
            fromTown->removeTraveler(this);
            fromTown = toTown;
            toTown->addTraveler(this);
            logText.push_back(
                name + " has arrived in the " +
                gameData.populationAdjectives.lower_bound(toTown->getProperty().getPopulation())->second +
                " " + toTown->getNation()->getAdjective() + " " +
                gameData.townTypeNames[static_cast<size_t>(toTown->getProperty().getTownType())] + " of " +
                toTown->getName() + ".");
        }
    }
    if (target) {
        if (fightWon() && aI) {
            aI->loot();
            loseTarget();
            return;
        }
        if (choice == FightChoice::fight) {
            double escapeChance;
            switch (target->choice) {
            case FightChoice::fight:
                fight(*target, elTm);
                break;
            case FightChoice::run:
                // Check if target escapes.
                escapeChance = Settings::getEscapeChance() * target->speed() / speed();
                if (Settings::random() > escapeChance) {
                    // Target is caught, fight.
                    target->choice = FightChoice::fight;
                    fight(*target, elTm);
                    std::string logEntry = name + " catches up to " + target->name + ".";
                    logText.push_back(logEntry);
                    target->logText.push_back(logEntry);
                } else {
                    // Target escapes.
                    std::string logEntry = target->name + " has eluded " + name + ".";
                    logText.push_back(logEntry);
                    target->logText.push_back(logEntry);
                    loseTarget();
                }
                break;
            default:
                break;
            }
        }
    }
}

void Traveler::toggleMaxGoods() { toTown->toggleMaxGoods(); }

void Traveler::resetTown() { toTown->reset(); }

void Traveler::adjustAreas(Pager &pgr, double mM) {
    std::vector<MenuButton *> requestButtons;
    std::vector<TextBox *> bxs = pgr.getAll();
    std::transform(begin(bxs), end(bxs), std::back_inserter(requestButtons),
                   [](auto rB) { return dynamic_cast<MenuButton *>(rB); });
    toTown->adjustAreas(requestButtons, mM);
}

void Traveler::adjustDemand(Pager &pgr, double mM) {
    // Adjust demand for goods in current town and show new prices on buttons.
    std::vector<MenuButton *> requestButtons;
    std::vector<TextBox *> bxs = pgr.getAll();
    std::transform(begin(bxs), end(bxs), std::back_inserter(requestButtons),
                   [](auto rB) { return dynamic_cast<MenuButton *>(rB); });
    toTown->adjustDemand(requestButtons, mM);
}
