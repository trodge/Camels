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

#include <numeric>
#include <unordered_map>
#include <vector>

#include <boost/multi_index/composite_key.hpp>
#include <boost/multi_index/hashed_index.hpp>
#include <boost/multi_index/mem_fun.hpp>
#include <boost/multi_index/member.hpp>
#include <boost/multi_index/ordered_index.hpp>
#include <boost/multi_index_container.hpp>
#include <boost/math/special_functions/relative_difference.hpp>
#include <boost/math/special_functions/next.hpp>

namespace mi = boost::multi_index;
namespace mt = boost::math;

#include "business.hpp"
#include "good.hpp"
#include "pager.hpp"

class Business;

struct GoodSum {
    double amount = 0;              // amount of the good available
    double maximum = 0;             // total space for this good
    unsigned int businessCount = 0; // number of businesses using the good
    double needed = 0;              // sum of the business need for the good
};

class Property {
    using GdId = mi::const_mem_fun<Good, unsigned int, &Good::getGoodId>;
    using MtId = mi::const_mem_fun<Good, unsigned int, &Good::getMaterialId>;
    using FlId = mi::const_mem_fun<Good, unsigned int, &Good::getFullId>;
    using FlIdOdr = mi::ordered_unique<mi::tag<struct IdOrder>, FlId>;
    using GdIdHsh = mi::hashed_non_unique<mi::tag<struct GoodId>, GdId>;
    using MtIdHsh = mi::hashed_unique<mi::tag<struct MaterialId>, mi::composite_key<Good, GdId, MtId>>;
    using FlIdHsh = mi::hashed_unique<mi::tag<struct FullId>, FlId>;
    using GoodContainer = boost::multi_index_container<Good, mi::indexed_by<FlIdOdr, GdIdHsh, MtIdHsh, FlIdHsh>>;
    double dayLength;
    unsigned int townType;
    bool coastal;
    unsigned long population;
    GoodContainer goods;
    std::vector<GoodSum> goodSums;
    std::vector<Business> businesses;
    int updateCounter;
    bool maxGoods = false;
    const Property *source = nullptr;
    void addGood(const Good &srGd, const std::function<void(Good &)> &fn);

public:
    Property(unsigned int tT, bool ctl, unsigned long ppl, const Property *src); // constructor for town
    Property(bool ctl, const Property *src)
        : dayLength(Settings::getDayLength()), coastal(ctl), source(src) {} // constructor for traveler
    Property(const std::vector<Good> &gds, const std::vector<Business> &bsns)
        : goods(begin(gds), end(gds)), businesses(bsns) {}    // constructor for nation
    Property(const Save::Property *ppt, const Property *src); // constructor for loading
    flatbuffers::Offset<Save::Property> save(flatbuffers::FlatBufferBuilder &b, unsigned int tId) const;
    unsigned int getTownType() const { return townType; }
    bool getCoastal() const { return coastal; }
    unsigned long getPopulation() const { return population; }
    bool hasGood(unsigned int fId) const;
    const Good &good(unsigned int fId) const { return *goods.get<FullId>().find(fId); }
    const Good &good(boost::tuple<unsigned int, unsigned int> gMId) const {
        return *goods.get<MaterialId>().find(gMId);
    }
    void queryGoods(const std::function<void(const Good &gd)> &fn) const;
    double amount(unsigned int gId) const;
    double maximum(unsigned int gId) const;
    double weight() const {
        return std::accumulate(begin(goods), end(goods), 0, [](double w, const auto &gd) { return w + gd.weight(); });
    }
    void setConsumption(const std::vector<std::array<double, 3>> &gdsCnsptn);
    void setFrequencies(const std::vector<double> &frqcs);
    void setMaximums();
    void toggleMaxGoods() { maxGoods = !maxGoods; }
    void scale(Good &gd);
    void reset();
    std::vector<Good> take(unsigned int gId, double amt);
    void take(Good &gd);
    void put(Good &gd);
    void use();
    void use(unsigned int gId, double amt);
    void create();
    void create(unsigned int opId, double amt);
    void create(unsigned int opId, unsigned int ipId, double amt);
    void build(const Business &bsn, double a);
    void demolish(const Business &bsn, double a);
    void update(unsigned int elTm);
    void buttons(Pager &pgr, const SDL_Rect &rt, BoxInfo &bI, Printer &pr,
                 const std::function<std::function<void(MenuButton *)>(const Good &)> &fn) const;
    void buttons(Pager &pgr, const SDL_Rect &rt, BoxInfo &bI, Printer &pr,
                 const std::function<std::function<void(MenuButton *)>(const Business &)> &fn) const;
    void adjustAreas(const std::vector<MenuButton *> &rBs, double d);
    void adjustDemand(const std::vector<MenuButton *> &rBs, double d);
    void saveFrequencies(std::string &u) const;
    void saveDemand(std::string &u) const;
};

#endif // INVENTORY_H
