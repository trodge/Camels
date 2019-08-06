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
#ifndef INVENTORY_H
#define INVENTORY_H

#include <unordered_map>
#include <vector>

#include "business.hpp"
#include "good.hpp"
#include "pager.hpp"

class Business;

class Inventory {
    std::vector<Good> goods;
    std::unordered_multimap<unsigned int, Good *> byGoodId;

public:
    Inventory(const std::vector<Good> &gds) : goods(gds) { mapGoods(); }
    Inventory(const Inventory &other) : goods(other.goods) { mapGoods(); }
    Inventory(const flatbuffers::Vector<flatbuffers::Offset<Save::Good>> *ldGds) {
        for (auto lGI = ldGds->begin(); lGI != ldGds->end(); ++lGI) goods.push_back(Good(*lGI));
        mapGoods();
    }
    flatbuffers::Offset<flatbuffers::Vector<flatbuffers::Offset<Save::Good>>> save(flatbuffers::FlatBufferBuilder &b) {
        return b.CreateVector<flatbuffers::Offset<Save::Good>>(goods.size(),
                                                               [this, &b](size_t i) { return goods[i].save(b); });
    }
    double getAmount(unsigned int gId) const {
        auto gdRng = byGoodId.equal_range(gId);
        return std::accumulate(gdRng.first, gdRng.second, 0, [](double tA, auto g) { return tA + g.second->getAmount(); });
    }
    double getMaximum(unsigned int gId) const {
        auto gdRng = byGoodId.equal_range(gId);
        return std::accumulate(gdRng.first, gdRng.second, 0,
                               [](double tA, auto g) { return tA + g.second->getMaximum(); });
    }
    unsigned int getGoodId(unsigned int fId) const { return goods[fId].getGoodId();}
    void mapGoods() {
        for (auto &gd : goods) byGoodId.emplace(gd.getGoodId(), &gd);
    }
    void setImages(const Inventory &other) {
        for (size_t i = 0; i < goods.size(); ++i) goods[i].setImage(other.goods[i].getImage());
    }
    void setConsumption(const std::vector<std::array<double, 3>> &gdsCnsptn);
    void setMaximum(const std::vector<Business> &bsns);
    std::vector<Good> take(unsigned int gId, double amt);
    void take(Good &gd) { goods[gd.getFullId()].take(gd); }
    void put(Good &gd) { goods[gd.getFullId()].put(gd); }
    void use() {
        for (auto &gd : goods) gd.use();
    }
    void use(unsigned int gId, double amt);
    void create() {
        for (auto &gd : goods) gd.create();
    }
    void create(unsigned int gId, double amt);
    void create(unsigned int iId, unsigned int oId, double amt);
    void update(unsigned int elTm) {
        for (auto &gd : goods) gd.update(elTm);
    }
    void update(unsigned int elTm, std::vector<Business> &bsns);
    void buttons(Pager &pgr, const SDL_Rect &rt, BoxInfo bI, Printer &pr,
                 const std::function<std::function<void(MenuButton *)>(const Good &)> &fn);
    void adjustDemand(const std::vector<MenuButton *> &rBs, double d);
    void saveDemand(unsigned long ppl, std::string &u) const {
        for (auto &gd : goods) gd.saveDemand(ppl, u);
    }
};

#endif // INVENTORY_H