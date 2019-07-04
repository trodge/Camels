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

#include "town.hpp"

Town::Town(sqlite3_stmt *q, const std::vector<Nation> &ns, const std::vector<Business> &bs,
           const std::map<std::pair<int, int>, double> &fFs, int fS) {
    // Load a town from the given sqlite query.
    canFocus = true;
    // Load town info.
    id = static_cast<unsigned int>(sqlite3_column_int(q, 0));
    std::vector<std::string> names;
    names.reserve(2);
    names.push_back(std::string(reinterpret_cast<const char *>(sqlite3_column_text(q, 1))));
    names.push_back(std::string(reinterpret_cast<const char *>(sqlite3_column_text(q, 2))));
    nation = &ns[static_cast<size_t>(sqlite3_column_int(q, 3) - 1)];
    longitude = sqlite3_column_double(q, 5);
    latitude = sqlite3_column_double(q, 4);
    coastal = bool(sqlite3_column_int(q, 6));
    population = static_cast<unsigned long>(sqlite3_column_int(q, 7));
    townType = static_cast<unsigned int>(sqlite3_column_int(q, 8));
    SDL_Rect rt = {0, 0, 0, 0};
    box = std::make_unique<TextBox>(rt, names, nation->getForeground(), nation->getBackground(), nation->getId(), true, 1, 1,
                                    fS);
    for (auto &b : bs) {
        double f = nation->getFrequency(b.getId(), b.getMode());
        if (f != 0 and (coastal or not b.getRequireCoast())) {
            businesses.push_back(Business(b));
            businesses.back().setArea(static_cast<double>(population) * f);
            auto fFI = fFs.find(std::make_pair(townType, b.getId()));
            if (fFI != fFs.end())
                businesses.back().setFrequencyFactor(fFI->second);
        }
    }
    // Copy goods list from nation.
    for (auto &g : nation->getGoods()) {
        goods.push_back(Good(g));
        goods.back().assignConsumption(population);
    }
    setMax();
    // Start with enough inputs for one run cycle.
    resetGoods();
    // randomize business run counter
    static std::uniform_int_distribution<unsigned int> dis(0, Settings::businessRunTime);
    businessCounter = static_cast<int>(dis(Settings::rng));
    travelersDis = std::uniform_int_distribution<>(Settings::travelersMin,
                                                   static_cast<int>(pow(population, Settings::travelersExponent)));
}

Town::Town(const Save::Town *t, const std::vector<Nation> &ns, int fS)
    : id(static_cast<unsigned int>(t->id())), nation(&ns[static_cast<size_t>(t->nation() - 1)]), longitude(t->longitude()),
      latitude(t->latitude()), coastal(t->coastal()), population(t->population()), townType(t->townType()),
      businessCounter(t->businessCounter()) {
    // Load a town from the given flatbuffers save object.
    canFocus = true;
    SDL_Rect rt = {0, 0, 0, 0};
    auto lNames = t->names();
    std::vector<std::string> names;
    names.push_back(lNames->Get(0)->str());
    names.push_back(lNames->Get(1)->str());
    box =
        std::make_unique<TextBox>(rt, names, nation->getForeground(), nation->getBackground(), nation->getId(), true, 1, fS);
    auto lBusinesses = t->businesses();
    for (auto lBI = lBusinesses->begin(); lBI != lBusinesses->end(); ++lBI)
        businesses.push_back(Business(*lBI));
    auto lGoods = t->goods();
    for (auto lGI = lGoods->begin(); lGI != lGoods->end(); ++lGI)
        goods.push_back(Good(*lGI));
    setMax();
}

bool Town::operator==(const Town &other) const { return id == other.id; }

int Town::distSq(int x, int y) const { return (dpx - x) * (dpx - x) + (dpy - y) * (dpy - y); }

double Town::dist(int x, int y) const { return sqrt(distSq(x, y)); }

void Town::setMax() {
    // Set maximum amounts for town goods.
    for (auto &g : goods)
        g.setMax();
    // Set good maximums for businesses.
    for (auto &b : businesses) {
        auto &ips = b.getInputs();
        auto &ops = b.getOutputs();
        for (auto &ip : ips) {
            if (ip == ops.back())
                // Livestock get full space for input amounts.
                goods[ip.getId()].setMax(ip.getAmount());
            else
                goods[ip.getId()].setMax(ip.getAmount() * Settings::inputSpaceFactor);
        }
        for (auto &op : ops)
            goods[op.getId()].setMax(op.getAmount() * Settings::outputSpaceFactor);
    }
}

void Town::removeTraveler(const std::shared_ptr<Traveler> t) {
    auto it = std::find(travelers.begin(), travelers.end(), t);
    if (it != travelers.end())
        travelers.erase(it);
}

void Town::placeDot(std::vector<SDL_Rect> &drawn, int ox, int oy, double s) {
    // Place the dot for this town based on the given offset and scale.
    dpx = int(s * longitude) + ox;
    dpy = oy - int(s * latitude);
    SDL_Rect r = {dpx - 3, r.y = dpy - 3, r.w = 6, r.h = 6};
    drawn.push_back(r);
}

void Town::drawRoutes(SDL_Renderer *s) {
    const SDL_Color &col = Settings::routeColor;
    SDL_SetRenderDrawColor(s, col.r, col.g, col.b, col.a);
    for (auto &n : neighbors) {
        SDL_RenderDrawLine(s, dpx, dpy, n->dpx, n->dpy);
    }
}

void Town::drawDot(SDL_Renderer *s) {
    const SDL_Rect &bR = box->getRect();
    SDL_Rect lR = {dpx, bR.y + bR.h, 1, dpy - bR.y - bR.h};
    const SDL_Color &fg = nation->getForeground();
    SDL_SetRenderDrawColor(s, fg.r, fg.g, fg.b, fg.a);
    SDL_RenderFillRect(s, &lR);
    const SDL_Color &dC = nation->getDotColor();
    drawCircle(s, dpx, dpy, 3, dC, true);
}

void Town::update(unsigned int e) {
    businessCounter += e;
    if (businessCounter > static_cast<int>(Settings::businessRunTime)) {
        for (auto &g : goods)
            g.consume(Settings::businessRunTime);
        std::vector<int> conflicts(goods.size(), 0);
        if (maxGoods) // When maxGoods is true, towns create as many goods as possible for testing purposes.
            for (auto &g : goods) {
                const std::vector<Material> &gMs = g.getMaterials();
                std::unordered_map<unsigned int, double> mAs; // Map of material ids to amounts of said material
                mAs.reserve(gMs.size());
                for (auto &gM : gMs)
                    if (g.getAmount() > 0.)
                        mAs.emplace(gM.getId(), g.getMax() * gM.getAmount() / g.getAmount());
                    else
                        mAs.emplace(gM.getId(), g.getMax() / static_cast<double>(gMs.size()));
                g.use();
                g.create(mAs);
            }
        for (auto &b : businesses) {
            // For each business, start by setting factor to business run time.
            b.setFactor(Settings::businessRunTime / static_cast<double>(kDaysPerYear * Settings::dayLength));
            // Count conflicts of businesses for available goods.
            b.addConflicts(conflicts, goods);
        }
        for (auto &b : businesses) {
            // Handle conflicts by reducing factors.
            b.handleConflicts(conflicts);
            // Run businesses on town's goods.
            b.run(goods);
        }
        businessCounter -= Settings::businessRunTime;
    }
}

void Town::take(Good &g) { goods[g.getId()].take(g); }

void Town::put(Good &g) { goods[g.getId()].put(g); }

void Town::generateTravelers(const GameData &gD, std::vector<std::shared_ptr<Traveler>> &ts) {
    // Create a random number of travelers from this town, weighted down
    int n = travelersDis(Settings::rng);
    if (n < 0)
        n = 0;
    ts.reserve(ts.size() + static_cast<size_t>(n));
    for (int i = 0; i < n; ++i)
        ts.push_back(std::make_shared<Traveler>(nation->randomName(), this, gD));
}

double Town::dist(const Town *t) const { return dist(t->dpx, t->dpy); }

void Town::loadNeighbors(std::vector<Town> &ts, const std::vector<size_t> &nIs) {
    // Load neighbors from vector of neighbor ids.
    neighbors.reserve(nIs.size());
    for (auto &nI : nIs)
        if (nI <= ts.size())
            neighbors.push_back(&ts[nI - 1]);
}

void Town::findNeighbors(std::vector<Town> &ts, SDL_Surface *mS, int mox, int moy) {
    // Find nearest towns that can be traveled to directly from this one on map surface.
    neighbors.clear();
    size_t max = 5;
    neighbors.reserve(max);
    auto closer = [this](Town *m, Town *n) { return distSq(m->dpx, m->dpy) < distSq(n->dpx, n->dpy); };
    for (auto &t : ts) {
        int dS = distSq(t.dpx, t.dpy);
        if (t.id != id and not std::binary_search(neighbors.begin(), neighbors.end(), &t, closer)) {
            // determine how much water is between these two towns
            int water = 0;
            // run incremental change to get from self to p
            double x = dpx;
            double y = dpy;
            double dx = t.dpx - dpx;
            double dy = t.dpy - dpy;
            double m;
            if (dx != 0.)
                m = dy / dx;
            double dt = 0;
            while (dt * dt < dS and water < 24) {
                if (dx != 0.) {
                    double dxs = 1 / (1 + m * m);
                    double dys = 1 - dxs;
                    x += copysign(sqrt(dxs), dx);
                    y += copysign(sqrt(dys), dy);
                    dt += sqrt(dxs + dys);
                } else {
                    y += 1;
                    dt += 1;
                }
                Uint8 r, g, b;
                int mx = static_cast<int>(x) + mox;
                int my = static_cast<int>(y) + moy;
                if (mx >= 0 and mx < mS->w and my >= 0 and my < mS->h)
                    SDL_GetRGB(getAt(mS, mx, my), mS->format, &r, &g, &b);
                else {
                    r = Settings::waterColor.r;
                    g = Settings::waterColor.g;
                    b = Settings::waterColor.b;
                }
                if (r <= Settings::waterColor.r and g <= Settings::waterColor.g and b >= Settings::waterColor.b) {
                    ++water;
                }
            }
            if (water < 24) {
                neighbors.insert(std::lower_bound(neighbors.begin(), neighbors.end(), &t, closer), &t);
            }
        }
    }
    // take only the closest neighbors
    if (max > neighbors.size())
        max = neighbors.size();
    neighbors =
        std::vector<Town *>(neighbors.begin(), neighbors.begin() + static_cast<std::vector<Town *>::difference_type>(max));
}

void Town::connectRoutes() {
    // Ensure that routes go both directions.
    auto closer = [this](Town *m, Town *n) { return distSq(m->dpx, m->dpy) < distSq(n->dpx, n->dpy); };
    for (auto &n : neighbors) {
        auto &nNs = n->neighbors;
        auto it = std::lower_bound(nNs.begin(), nNs.end(), n, closer);
        if (it == nNs.end() or *it != this)
            nNs.insert(it, this);
    }
}

void Town::saveNeighbors(std::string &i) const {
    for (auto &n : neighbors) {
        i.append(" (");
        i.append(std::to_string(id));
        i.append(", ");
        i.append(std::to_string(n->id));
        i.append("),");
    }
}

void Town::adjustAreas(const std::vector<MenuButton *> &rBs, double d) {
    for (auto &b : businesses) {
        bool go = false;
        bool inputMatch = false;
        std::vector<std::string> mMs = {"wool", "hide", "milk", "bowstring", "arrowhead", "cloth"};
        for (auto &rB : rBs) {
            if (rB->getClicked()) {
                std::string rBN = rB->getText()[0];
                for (auto &ip : b.getInputs()) {
                    std::string ipN = goods[ip.getId()].getName();
                    if (rBN.substr(0, rBN.find(' ')) == ipN.substr(0, ipN.find(' ')) or
                        std::find(mMs.begin(), mMs.end(), ipN) != mMs.end()) {
                        inputMatch = true;
                        break;
                    }
                }
                for (auto &op : b.getOutputs()) {
                    if ((not rB->getId() or op.getId() == rB->getId()) and (not b.getKeepMaterial() or inputMatch)) {
                        go = true;
                        break;
                    }
                }
            }
            if (go)
                break;
        }
        if ((go) and b.getArea() > -d * static_cast<double>(population) / 5000) {
            b.setArea(b.getArea() + d * static_cast<double>(population) / 5000);
        }
    }
    setMax();
}

void Town::resetGoods() {
    for (auto &g : goods)
        g.use();
    for (auto &b : businesses)
        for (auto &ip : b.getInputs())
            if (ip == b.getOutputs().back())
                goods[ip.getId()].create(ip.getAmount());
            else
                goods[ip.getId()].create(ip.getAmount() * Settings::businessRunTime /
                                         static_cast<double>(Settings::dayLength * kDaysPerYear));
}

void Town::saveFrequencies(std::string &u) const {
    for (auto &b : businesses) {
        b.saveFrequency(population, u);
    }
    u.append(" ELSE frequency END WHERE nation_id = ");
    u.append(std::to_string(nation->getId()));
}

void Town::adjustDemand(const std::vector<MenuButton *> &rBs, double d) {
    for (auto &rB : rBs)
        if (rB->getClicked()) {
            std::string rBN = rB->getText()[0];
            goods[rB->getId()].adjustDemand(rBN.substr(0, rBN.find(' ')), d / static_cast<double>(population) / 1000);
        }
}

void Town::saveDemand(std::string &u) const {
    for (auto &g : goods)
        g.saveDemand(population, u);
    u.append(" ELSE demand_slope END WHERE nation_id = ");
    u.append(std::to_string(nation->getId()));
}

flatbuffers::Offset<Save::Town> Town::save(flatbuffers::FlatBufferBuilder &b) const {
    auto sNames = b.CreateVectorOfStrings(box->getText());
    auto sBusinesses = b.CreateVector<flatbuffers::Offset<Save::Business>>(
        businesses.size(), [this, &b](size_t i) { return businesses[i].save(b); });
    auto sGoods =
        b.CreateVector<flatbuffers::Offset<Save::Good>>(goods.size(), [this, &b](size_t i) { return goods[i].save(b); });
    auto sNeighbors = b.CreateVector<unsigned int>(neighbors.size(), [this](size_t i) { return neighbors[i]->getId(); });
    return Save::CreateTown(b, id, sNames, nation->getId(), longitude, latitude, coastal, population, townType, sBusinesses,
                            sGoods, sNeighbors, businessCounter);
}
