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
           TownType tT, bool ctl, unsigned long ppl, Printer &pr)
    : id(i), nation(nt),
      box(std::make_unique<TextBox>(Settings::boxInfo({0, 0, 0, 0}, nms, nt->getColors(), {nt->getId(), true},
                                                      BoxSizeType::town, BoxBehavior::focus),
                                    pr)),
      position(lng, lat), property(tT, ctl, ppl, &nt->getProperty()) {}

Town::Town(const Save::Town *ldTn, const std::vector<Nation> &ns, Printer &pr)
    : id(static_cast<unsigned int>(ldTn->id())), nation(&ns[static_cast<size_t>(ldTn->nation() - 1)]),
      box(std::make_unique<TextBox>(
          Settings::boxInfo({0, 0, 0, 0}, {ldTn->names()->Get(0)->str(), ldTn->names()->Get(1)->str()},
                            nation->getColors(), {nation->getId(), true}, BoxSizeType::town, BoxBehavior::focus),
          pr)),
      position(ldTn->longitude(), ldTn->latitude()), property(ldTn->property(), &nation->getProperty()) {
    // Load a town from the given flatbuffers save object. Copy image pointers from nation's goods.
}

flatbuffers::Offset<Save::Town> Town::save(flatbuffers::FlatBufferBuilder &b) const {
    auto svNames = b.CreateVectorOfStrings(box->getText());
    return Save::CreateTown(b, id, svNames, nation->getId(), position.getLongitude(), position.getLatitude(),
                            property.save(b, id));
}

bool Town::operator==(const Town &other) const { return id == other.id; }

void Town::removeTraveler(const Traveler *t) {
    auto it = std::find(begin(travelers), end(travelers), t);
    if (it != end(travelers)) travelers.erase(it);
}

void Town::placeDot(std::vector<SDL_Rect> &drawn, const SDL_Point &ofs, double s) {
    // Place the dot for this town based on the given offset and scale.
    position.place(ofs, s);
    auto &point = position.getPoint();
    drawn.push_back({point.x - 3, point.y - 3, 6, 6});
}

void Town::draw(SDL_Renderer *s) {
    const SDL_Rect &bR = box->getRect();
    auto &point = position.getPoint();
    SDL_Rect lR = {point.x, bR.y + bR.h, 1, point.y - bR.y - bR.h};
    const SDL_Color &fg = nation->getColors().foreground;
    SDL_SetRenderDrawColor(s, fg.r, fg.g, fg.b, fg.a);
    SDL_RenderFillRect(s, &lR);
    const SDL_Color &dC = nation->getDotColor();
    drawCircle(s, point, 3, dC, true);
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

int Town::distSq(const Town *t) const { return position.distSq(t->position); }

void Town::findNeighbors(std::vector<Town> &ts, SDL_Surface *mS, const SDL_Point &mOs) {
    // Find nearest towns that can be traveled to directly from this one on map surface.
    neighbors.clear();
    size_t mN = Settings::getMaxNeighbors();
    neighbors.reserve(mN);
    auto closer = [this](Town *m, Town *n) {
        return position.distSq(m->position) < position.distSq(n->position);
    };
    for (auto &t : ts) {
        int dS = position.distSq(t.position);
        if (t.id != id && !std::binary_search(begin(neighbors), end(neighbors), &t, closer)) {
            // t is not this and not already a neighbor.
            // Determine how much water is between these two towns.
            unsigned int water = 0;
            // Run incremental change to get from self to t.
            auto &point = position.getPoint();
            double x = point.x;
            double y = point.y;
            auto &tnPoint = t.position.getPoint();
            double dx = tnPoint.x - point.x;
            double dy = tnPoint.y - point.y;
            bool swap = dx == 0;
            if (swap) {
                std::swap(x, y);
                std::swap(dx, dy);
            }
            double m = dy / dx;
            double dt = 0; // distance traveled
            while (dt * dt < dS && water < kMaxWater) {
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
                SDL_Point m = {static_cast<int>(x) + mOs.x, static_cast<int>(y) + mOs.y};
                if (m.x >= 0 && m.x < mS->w && m.y >= 0 && m.y < mS->h) {
                    const SDL_Color &waterColor = Settings::getWaterColor();
                    Uint8 r, g, b;
                    SDL_GetRGB(getAt(mS, m), mS->format, &r, &g, &b);
                    if (r <= waterColor.r && g <= waterColor.g && b >= waterColor.b) ++water;
                } else
                    // SDL_Point is out of map bounds.
                    ++water;
            }
            if (water < kMaxWater) {
                neighbors.insert(std::lower_bound(begin(neighbors), end(neighbors), &t, closer), &t);
            }
        }
    }
    // Take only the closest towns.
    if (neighbors.size() > mN) neighbors = std::vector<Town *>(begin(neighbors), begin(neighbors) + mN);
}

void Town::connectRoutes() {
    // Ensure that routes go both directions.
    auto closer = [this](Town *m, Town *n) {
        return position.distSq(m->position) < position.distSq(n->position);
    };
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
    auto &pt = towns[0]->getPosition().getPoint(), &qt = towns[1]->getPosition().getPoint();
    SDL_RenderDrawLine(s, pt.x, pt.y, qt.x, qt.y);
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
