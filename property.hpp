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

namespace mi = boost::multi_index;

#include "business.hpp"
#include "constants.hpp"
#include "good.hpp"
#include "pager.hpp"

struct Conflict {
    unsigned int count = 0; // number of businesses using the good
    double needed = 0;      // sum of the business need for the good
    bool conflicted = false;
};

class Business;

struct GoodBalance {
    const Good *cheapest = nullptr;
    double amount, price, ratio;
};

struct BusinessPlan;

class Property {
    using GdId = mi::const_mem_fun<Good, unsigned int, &Good::getGoodId>;
    using MtId = mi::const_mem_fun<Good, unsigned int, &Good::getMaterialId>;
    using FlId = mi::const_mem_fun<Good, unsigned int, &Good::getFullId>;
    using FlIdOdr = mi::ordered_unique<mi::tag<struct IdOrder>, FlId>;
    using GdIdHsh = mi::hashed_non_unique<mi::tag<struct GoodId>, GdId>;
    using MtIdHsh = mi::hashed_unique<mi::tag<struct MaterialId>, mi::composite_key<Good, GdId, MtId>>;
    using FlIdHsh = mi::hashed_unique<mi::tag<struct FullId>, FlId>;
    using GoodContainer = boost::multi_index_container<Good, mi::indexed_by<FlIdOdr, GdIdHsh, MtIdHsh, FlIdHsh>>;
    TownType townType;
    bool coastal;
    unsigned long population;
    GoodContainer goods;
    std::vector<Business> businesses;
    int updateCounter;
    bool maxGoods = false;
    const Property *source = nullptr;
    BusinessPlan buildPlan(const Business &bsn, const Property &tvlPpt, double ofVl) const;
    BusinessPlan restockPlan(const Business &bsn, const Property &tvlPpt, double ofVl) const;
    void addGood(const Good &srGd, const std::function<void(Good &)> &fn);

public:
    Property(TownType tT, bool ctl, unsigned long ppl, const Property *src); // constructor for town
    Property(bool ctl, const Property *src)
        : coastal(ctl), population(0), updateCounter(Settings::propertyUpdateCounter()), source(src) {} // constructor for traveler
    Property(const std::vector<Good> &gds, const std::vector<Business> &bsns)
        : goods(begin(gds), end(gds)), businesses(bsns) {}      // constructor for nation
    Property(const Save::Property *svPpt, const Property *src); // constructor for loading
    flatbuffers::Offset<Save::Property> save(flatbuffers::FlatBufferBuilder &b, unsigned int tId) const;
    TownType getTownType() const { return townType; }
    bool getCoastal() const { return coastal; }
    unsigned long getPopulation() const { return population; }
    const std::vector<Business> &getBusinesses() const { return businesses; }
    bool hasGood(unsigned int fId) const;
    const Good *good(unsigned int fId) const;
    const Good *good(boost::tuple<unsigned int, unsigned int> gMId) const;
    const std::vector<unsigned int> fullIds() const;
    void forGood(const std::function<void(const Good &gd)> &fn) const;
    void forGood(unsigned int gId, const std::function<void(const Good &gd)> &fn) const;
    double amount(unsigned int gId) const;
    double maximum(unsigned int gId) const;
    double weight() const;
    std::pair<const Good *, double> cheapest(unsigned int gId) const;
    double balance(std::vector<Good> &gds, const Property &tvlPpt, double &cst) const;
    std::vector<BusinessPlan> buildPlans(const Property &tvlPpt, double ofVl) const;
    std::vector<BusinessPlan> restockPlans(const Property &tvlPpt, const Property &srgPpt, double ofVl) const;
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
    void input(unsigned int ipId, double amt);
    void create(unsigned int fId, double amt);
    void create();
    void output(unsigned int opId, double amt);
    void output(unsigned int opId, unsigned int ipId, double amt);
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
