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

Town::Town(unsigned int i, const std::vector<std::string> &nms, const Nation *nt, double lng, double lat,
           unsigned int tT, bool ctl, unsigned long ppl, Printer &pr)
    : id(i), nation(nt),
      box(std::make_unique<TextBox>(
          Settings::boxInfo({0, 0, 0, 0}, nms, nt->getColors(), nt->getId(), true, true, false, BoxSize::town), pr)),
      longitude(lng), latitude(lat), property(tT, ctl, ppl, &nt->getProperty()) {}

Town::Town(const Save::Town *t, const std::vector<Nation> &ns, Printer &pr)
    : id(static_cast<unsigned int>(t->id())), nation(&ns[static_cast<size_t>(t->nation() - 1)]),
      box(std::make_unique<TextBox>(Settings::boxInfo({0, 0, 0, 0}, {t->names()->Get(0)->str(), t->names()->Get(1)->str()},
                                                      nation->getColors(), nation->getId(), true, true, false, BoxSize::town),
                                    pr)),
      longitude(t->longitude()), latitude(t->latitude()), property(t->property(), &nation->getProperty()) {
    // Load a town from the given flatbuffers save object. Copy image pointers from nation's goods.
}

flatbuffers::Offset<Save::Town> Town::save(flatbuffers::FlatBufferBuilder &b) const {
    auto svNames = b.CreateVectorOfStrings(box->getText());
    return Save::CreateTown(b, id, svNames, nation->getId(), longitude, latitude, property.save(b, id));
}

bool Town::operator==(const Town &other) const { return id == other.id; }

int Town::distSq(int x, int y) const { return (dpx - x) * (dpx - x) + (dpy - y) * (dpy - y); }

double Town::dist(int x, int y) const { return sqrt(distSq(x, y)); }

void Town::removeTraveler(const Traveler *t) {
    auto it = std::find(begin(travelers), end(travelers), t);
    if (it != end(travelers)) travelers.erase(it);
}

void Town::placeDot(std::vector<SDL_Rect> &drawn, int ox, int oy, double s) {
    // Place the dot for this town based on the given offset and scale.
    dpx = int(s * longitude) + ox;
    dpy = oy - int(s * latitude);
    SDL_Rect r = {dpx - 3, r.y = dpy - 3, r.w = 6, r.h = 6};
    drawn.push_back(r);
}

void Town::draw(SDL_Renderer *s) {
    const SDL_Rect &bR = box->getRect();
    SDL_Rect lR = {dpx, bR.y + bR.h, 1, dpy - bR.y - bR.h};
    const SDL_Color &fg = nation->getColors().foreground;
    SDL_SetRenderDrawColor(s, fg.r, fg.g, fg.b, fg.a);
    SDL_RenderFillRect(s, &lR);
    const SDL_Color &dC = nation->getDotColor();
    drawCircle(s, dpx, dpy, 3, dC, true);
}

void Town::update(unsigned int elTm) { property.update(elTm); }

void Town::take(Good &g) { property.take(g); }

void Town::put(Good &g) { property.put(g); }

void Town::generateTravelers(const GameData &gD, std::vector<std::unique_ptr<Traveler>> &tvlrs) {
    // Create a random number of travelers from this town, weighted down.
    size_t n = Settings::travelerCount(property.getPopulation());
    tvlrs.reserve(tvlrs.size() + n);
    for (int i = 0; i < n; ++i) tvlrs.push_back(std::make_unique<Traveler>(nation->randomName(), this, gD));
}

double Town::dist(const Town *t) const { return dist(t->dpx, t->dpy); }

void Town::findNeighbors(std::vector<Town> &ts, SDL_Surface *mS, int mox, int moy) {
    // Find nearest towns that can be traveled to directly from this one on map surface.
    neighbors.clear();
    size_t mN = Settings::getMaxNeighbors();
    neighbors.reserve(mN);
    auto closer = [this](Town *m, Town *n) { return distSq(m->dpx, m->dpy) < distSq(n->dpx, n->dpy); };
    for (auto &t : ts) {
        int dS = distSq(t.dpx, t.dpy);
        if (t.id != id && !std::binary_search(begin(neighbors), end(neighbors), &t, closer)) {
            // t is not this and not already a neighbor.
            // Determine how much water is between these two towns.
            int water = 0;
            // Run incremental change to get from self to t.
            double x = dpx;
            double y = dpy;
            double dx = t.dpx - dpx;
            double dy = t.dpy - dpy;
            bool swap = dx == 0;
            if (swap) {
                std::swap(x, y);
                std::swap(dx, dy);
            }
            double m = dy / dx;
            double dt = 0; // distance traveled
            while (dt * dt < dS && water < 24) {
                if (dx != 0) {
                    double dxs = 1 / (1 + m * m);
                    double dys = 1 - dxs;
                    x += copysign(sqrt(dxs), dx);
                    y += copysign(sqrt(dys), dy);
                    dt += sqrt(dxs + dys);
                } else {
                    y += 1;
                    dt += 1;
                }
                int mx = static_cast<int>(x) + mox;
                int my = static_cast<int>(y) + moy;
                if (mx >= 0 && mx < mS->w && my >= 0 && my < mS->h) {
                    const SDL_Color &waterColor = Settings::getWaterColor();
                    Uint8 r, g, b;
                    SDL_GetRGB(getAt(mS, mx, my), mS->format, &r, &g, &b);
                    if (r <= waterColor.r && g <= waterColor.g && b >= waterColor.b) ++water;
                } else
                    ++water;
            }
            if (water < 24) { neighbors.insert(std::lower_bound(begin(neighbors), end(neighbors), &t, closer), &t); }
        }
    }
    // Take only the closest towns.
    if (neighbors.size() > mN) neighbors = std::vector<Town *>(begin(neighbors), begin(neighbors) + mN);
}

void Town::connectRoutes() {
    // Ensure that routes go both directions.
    auto closer = [this](Town *m, Town *n) { return distSq(m->dpx, m->dpy) < distSq(n->dpx, n->dpy); };
    for (auto &n : neighbors) {
        auto &nNs = n->neighbors; // neighbor's neighbors
        auto it = std::lower_bound(begin(nNs), end(nNs), n, closer);
        if (it == end(nNs) || *it != this) nNs.insert(it, this);
    }
}

void Town::saveFrequencies(std::string &u) const {
    property.saveFrequencies(u);
    u.append(" ELSE frequency END WHERE nation_id = ");
    u.append(std::to_string(nation->getId()));
}

void Town::saveDemand(std::string &u) const {
    property.saveDemand(u);
    u.append(" ELSE demand_slope END WHERE nation_id = ");
    u.append(std::to_string(nation->getId()));
}

Route::Route(Town *fT, Town *tT) : towns({fT, tT}) {}

Route::Route(const Save::Route *rt, std::vector<Town> &ts) {
    auto lTowns = rt->towns();
    for (size_t i = 0; i < 2; ++i) towns[1] = &ts[lTowns->Get(1)];
}

void Route::draw(SDL_Renderer *s) {
    const SDL_Color &col = Settings::getRouteColor();
    SDL_SetRenderDrawColor(s, col.r, col.g, col.b, col.a);
    SDL_RenderDrawLine(s, towns[0]->getDPX(), towns[0]->getDPY(), towns[1]->getDPX(), towns[1]->getDPY());
}

void Route::saveData(std::string &i) const {
    i.append(" (");
    i.append(std::to_string(towns[0]->getId()));
    i.append(", ");
    i.append(std::to_string(towns[1]->getId()));
    i.append("),");
}

flatbuffers::Offset<Save::Route> Route::save(flatbuffers::FlatBufferBuilder &b) const {
    auto sTowns = b.CreateVector<unsigned int>(towns.size(), [this](size_t i) { return towns[i]->getId(); });
    return Save::CreateRoute(b, sTowns);
}
