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
#ifndef PROPERTY_H
#define PROPERTY_H

#include <unordered_map>
#include <vector>

#include "business.hpp"
#include "good.hpp"
#include "pager.hpp"

class Business;

class Property {
    unsigned int id;
    std::vector<Good> goods;
    std::unordered_multimap<unsigned int, Good *> byGoodId;
    std::vector<Business> businesses;

public:
    Property(unsigned int i, const std::vector<Good> &gds, const std::vector<Business> &bsns)
        : id(i), goods(gds), businesses(bsns) {
        mapGoods();
    }
    Property(unsigned int i, const std::vector<Good> &gds) : Property(i, gds, {}) {}
    Property(unsigned int i, const Property &other) : Property(i, other.goods, other.businesses) {}
    Property(const Save::Property *ppt, const Property &other);
    flatbuffers::Offset<Save::Property> save(flatbuffers::FlatBufferBuilder &b) const;
    unsigned int getId() const { return id; }
    unsigned int getGoodId(unsigned int fId) const { return goods[fId].getGoodId(); }
    double getAmount(unsigned int gId) const {
        auto gdRng = byGoodId.equal_range(gId);
        return std::accumulate(gdRng.first, gdRng.second, 0, [](double tA, auto g) { return tA + g.second->getAmount(); });
    }
    double getMaximum(unsigned int gId) const {
        auto gdRng = byGoodId.equal_range(gId);
        return std::accumulate(gdRng.first, gdRng.second, 0,
                               [](double tA, auto g) { return tA + g.second->getMaximum(); });
    }
    const std::vector<Good> &getGoods() const { return goods; }
    double weight() const {
        return std::accumulate(begin(goods), end(goods), 0, [](double d, const auto &g) { return d + g.weight(); });
    }
    void mapGoods() {
        for (auto &gd : goods) byGoodId.emplace(gd.getGoodId(), &gd);
    }
    void setConsumption(const std::vector<std::array<double, 3>> &gdsCnsptn);
    void setFrequencies(const std::vector<double> &frqcs);
    void setMaximums();
    void setAreas(bool ctl, unsigned long ppl, unsigned int tT, const std::map<std::pair<unsigned int, unsigned int>, double> &fFs);
    std::vector<Good> take(unsigned int gId, double amt);
    void take(Good &gd) { goods[gd.getFullId()].take(gd); }
    void put(Good &gd) { goods[gd.getFullId()].put(gd); }
    void use(unsigned int gId, double amt);
    void create() {
        for (auto &gd : goods) gd.create();
    }
    void create(unsigned int gId, double amt);
    void create(unsigned int iId, unsigned int oId, double amt);
    void build(const Business &bsn, double a);
    void demolish(const Business &bsn, double a);
    void update(unsigned int elTm);
    void reset();
    void buttons(Pager &pgr, const SDL_Rect &rt, BoxInfo &bI, Printer &pr,
                 const std::function<std::function<void(MenuButton *)>(const Good &)> &fn) const;
    void buttons(Pager &pgr, const SDL_Rect &rt, BoxInfo &bI, Printer &pr,
                 const std::function<std::function<void(MenuButton *)>(const Business &)> &fn) const;
    void adjustAreas(const std::vector<MenuButton *> &rBs, double d);
    void adjustDemand(const std::vector<MenuButton *> &rBs, double d);
    void saveFrequencies(unsigned long ppl, std::string &u) const;
    void saveDemand(unsigned long ppl, std::string &u) const;
};

#endif // INVENTORY_H
