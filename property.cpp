#include "property.hpp"

Property::Property(TownType tT, bool ctl, unsigned long ppl, const Property *src)
    : townType(tT), coastal(ctl), population(ppl), updateCounter(Settings::propertyUpdateCounter()), source(src) {
    // Copy businesses from source that can exist here given coastal.
    std::copy_if(begin(source->businesses), end(source->businesses), std::back_inserter(businesses),
                 [ctl](auto &bsn) { return bsn.getFrequency() > 0 && (!bsn.getRequireCoast() || ctl); });
    // Scale business areas according to town population and type.
    for (auto &bsn : businesses) bsn.scale(ppl, tT);
    // Create starting goods.
    reset();
}

Property::Property(const Save::Property *svPpt, const Property *src)
    : townType(static_cast<TownType>(svPpt->townType())), coastal(svPpt->coastal()),
      population(svPpt->population()), updateCounter(svPpt->updateCounter()), source(src) {
    auto ldGds = svPpt->goods();
    std::transform(ldGds->begin(), ldGds->end(), std::inserter(goods, end(goods)),
                   [](auto ldGd) { return Good(ldGd); });
    for (auto gdIt = begin(goods); gdIt != end(goods); ++gdIt)
        goods.modify(gdIt,
                     [srGd = source->good(gdIt->getFullId())](auto &gd) { gd.setImage(srGd->getImage()); });
    auto ldBsns = svPpt->businesses();
    std::transform(ldBsns->begin(), ldBsns->end(), std::back_inserter(businesses),
                   [](auto ldBsn) { return Business(ldBsn); });
    setMaximums();
}

flatbuffers::Offset<Save::Property> Property::save(flatbuffers::FlatBufferBuilder &b, unsigned int tId) const {
    std::vector<Good> gds(begin(goods), end(goods));
    auto svGoods = b.CreateVector<flatbuffers::Offset<Save::Good>>(
        goods.size(), [&b, &gds](size_t i) { return gds[i].save(b); });
    auto svBusinesses = b.CreateVector<flatbuffers::Offset<Save::Business>>(
        businesses.size(), [this, &b](size_t i) { return businesses[i].save(b); });
    return Save::CreateProperty(b, tId, static_cast<Save::TownType>(townType), coastal, population, svGoods,
                                svBusinesses, updateCounter);
}

bool Property::hasGood(unsigned int fId) const {
    auto &byFullId = goods.get<FullId>();
    return byFullId.find(fId) != end(byFullId);
}

const Good *Property::good(unsigned int fId) const {
    auto &byFullId = goods.get<FullId>();
    auto gdIt = byFullId.find(fId);
    if (gdIt == end(byFullId)) return nullptr;
    return &*gdIt;
}

const Good *Property::good(boost::tuple<unsigned int, unsigned int> gMId) const {
    auto &byMaterialId = goods.get<MaterialId>();
    auto gdIt = byMaterialId.find(gMId);
    if (gdIt == end(byMaterialId)) return nullptr;
    return &*gdIt;
}

const std::vector<unsigned int> Property::fullIds() const {
    std::vector<unsigned int> fIds;
    fIds.reserve(goods.size());
    for (auto &gd : goods) fIds.push_back(gd.getFullId());
    return fIds;
}

void Property::forGood(const std::function<void(const Good &gd)> &fn) const {
    // Run given function for each good.
    std::for_each(begin(goods), end(goods), fn);
}

void Property::forGood(unsigned int gId, const std::function<void(const Good &gd)> &fn) const {
    // Run given function for each good with given good id.
    auto rng = goods.get<GoodId>().equal_range(gId);
    std::for_each(rng.first, rng.second, fn);
}

double Property::amount(unsigned int gId) const {
    auto gdRng = goods.get<GoodId>().equal_range(gId);
    return std::accumulate(gdRng.first, gdRng.second, 0.,
                           [](double tt, auto &gd) { return tt + gd.getAmount(); });
}

double Property::maximum(unsigned int gId) const {
    auto gdRng = goods.get<GoodId>().equal_range(gId);
    return std::accumulate(gdRng.first, gdRng.second, 0.,
                           [](double tt, auto &gd) { return tt + gd.getMaximum(); });
}

double Property::weight() const {
    return std::accumulate(begin(goods), end(goods), 0,
                           [](double w, const auto &gd) { return w + gd.weight(); });
}

double Property::balance(std::vector<Good> &gds, const Property &tvlPpt, double &cst) const {
    // Set amounts of given goods such that they can be purchased for given cost, keeping ratios the
    // same. Adjusts cost downward and sets full ids, names, and measure words for goods.
    double factor = std::numeric_limits<double>::max();
    if (cst == 0) {
        // No goods can be bought.
        for (auto &gd : goods) factor = std::min(tvlPpt.amount(gd.getGoodId()) / gd.getAmount(), factor);
        gds.clear();
        return factor;
    }
    double amountDotProduct = 0, ratioDotProduct = 0; // dot product of amounts and prices and prices and ratios
    size_t goodCount = gds.size();
    std::vector<GoodBalance> goodBalances(goodCount);
    for (size_t i = 0; i < goodCount; ++i) {
        auto &gd = gds[i];
        auto gdId = gd.getGoodId();
        auto rng = goods.get<GoodId>().equal_range(gdId);
        auto &blnc = goodBalances[i];
        blnc.amount = tvlPpt.amount(gdId);
        blnc.price = std::numeric_limits<double>::max();
        blnc.ratio = gd.getAmount();
        std::for_each(rng.first, rng.second, [&blnc](const Good &tnGd) {
            if (tnGd.getAmount() > 0) {
                double price = tnGd.price();
                if (price < blnc.price) {
                    blnc.price = price;
                    blnc.cheapest = &tnGd;
                }
            }
        });
        if (!blnc.cheapest) /* A good is not available */
            return 0;
        gd.setFullId(blnc.cheapest->getFullId());
        gd.setFullName(blnc.cheapest->getFullName());
        gd.setMeasure(blnc.cheapest->getMeasure());
        amountDotProduct += blnc.amount * blnc.price;
        ratioDotProduct += blnc.ratio * blnc.price;
    }
    double totalCost = 0;
    for (size_t i = 0; i < goodCount; ++i) {
        auto &gd = gds[i];
        // Set amount to linear estimate.
        auto &blnc = goodBalances[i];
        double linearEstimate = std::max((blnc.ratio * (amountDotProduct + cst - blnc.amount * blnc.price) -
                                          blnc.amount * (ratioDotProduct - blnc.price * blnc.ratio)) /
                                             ratioDotProduct,
                                         0.);
        // Add cost of linear estimate to total cost.
        totalCost += blnc.cheapest->cost(linearEstimate);
        // Set amount to linear estimate.
        gd.setAmount(linearEstimate);
    }
    if (totalCost > cst) {
        // Quantities need to be adjusted down.
        double adjustment = cst / totalCost;
        cst = 0;
        for (size_t i = 0; i < goodCount; ++i) {
            auto &gd = gds[i];
            // Adjust amount.
            double amount = gd.getAmount() * adjustment;
            gd.setAmount(amount);
            auto &blnc = goodBalances[i];
            factor = std::min((amount + blnc.amount) / blnc.ratio, factor);
            // Add cost of new amount to output variable.
            cst += blnc.cheapest->cost(amount);
        }
    } else {
        cst = totalCost;
        for (size_t i = 0; i < goodCount; ++i) {
            auto &blnc = goodBalances[i];
            factor = std::min((gds[i].getAmount() + blnc.amount) / blnc.ratio, factor);
        }
    }
    return factor;
}

BuildPlan Property::buildPlan(const Business &bsn, const Property &tvlPpt, double ofVl) const {
    // Add a build plan for given business to given build plan vector if it can be built.
    BuildPlan bdp{bsn};
    for (auto &rq : bsn.getRequirements()) bdp.request.push_back(rq);
    double area = bsn.getArea();
    for (auto &ip : bsn.getInputs()) {
        bdp.request.push_back(ip);
        bdp.request.back().setAmount(ip.getAmount() / area);
    }
    bdp.area = balance(bdp.request, tvlPpt, ofVl);
    bdp.cost = ofVl;
    return bdp;
}

std::vector<BuildPlan> Property::buildPlans(const Property &tvlPpt, double ofVl) const {
    // Return vector of plans for businesses that can be built with given starting goods.
    std::vector<BuildPlan> bdb;
    bdb.reserve(businesses.size());
    for (auto &bsn : businesses) {
        // Add build plan for all businesses.
        auto bdp = buildPlan(bsn, tvlPpt, ofVl);
        if (bdp.area > 0) bdb.push_back(bdp);
    }
    return bdb;
}

void Property::setConsumption(const std::vector<std::array<double, 3>> &gdsCnsptn) {
    for (auto gdIt = begin(goods); gdIt != end(goods); ++gdIt)
        goods.modify(gdIt, [cnsptn = gdsCnsptn[gdIt->getFullId()]](auto &gd) { gd.setConsumption(cnsptn); });
}

void Property::setFrequencies(const std::vector<double> &frqcs) {
    for (size_t i = 0; i < frqcs.size(); ++i) businesses[i].setFrequency(frqcs[i]);
}

void Property::setMaximums() {
    // Set maximum good amounts given businesses.
    for (auto gdIt = begin(goods); gdIt != end(goods); ++gdIt)
        goods.modify(gdIt, [](auto &gd) { gd.setMaximum(); });
    // Set good maximums for businesses.
    auto &byGoodId = goods.get<GoodId>();
    for (auto &b : businesses) {
        auto &ips = b.getInputs();
        auto &ops = b.getOutputs();
        double max;
        for (auto &ip : ips) {
            auto gdRng = byGoodId.equal_range(ip.getGoodId());
            if (ip == ops.back())
                // Livestock get full space for input amounts.
                max = ip.getAmount();
            else
                max = ip.getAmount() * Settings::getInputSpaceFactor();
            for (auto gdIt = gdRng.first; gdIt != gdRng.second; ++gdIt)
                byGoodId.modify(gdIt, [max](auto &gd) { gd.setMaximum(max); });
        }
        for (auto &op : ops) {
            auto gdRng = byGoodId.equal_range(op.getGoodId());
            max = op.getAmount() * Settings::getOutputSpaceFactor();
            for (auto gdIt = gdRng.first; gdIt != gdRng.second; ++gdIt)
                byGoodId.modify(gdIt, [max](auto &gd) { gd.setMaximum(max); });
        }
    }
    for (auto gdIt = begin(goods); gdIt != end(goods); ++gdIt)
        goods.modify(gdIt, [](auto &gd) { gd.setDemandSlope(); });
}

void Property::addGood(const Good &srGd, const std::function<void(Good &)> &fn) {
    // Add a new good to this property after scaling it, setting its maximum, then calling parameter function on it.
    auto gd = Good(srGd);
    gd.scale(population);
    gd.setMaximum();
    auto gId = gd.getGoodId();
    auto same = [gId](const Good &gd) { return gd.getGoodId() == gId; }; // function to look for good in inputs and outputs
    auto ipSpF = Settings::getInputSpaceFactor();
    auto opSpF = Settings::getOutputSpaceFactor();
    for (auto &bsn : businesses) {
        auto &ips = bsn.getInputs();
        auto &ops = bsn.getOutputs();
        auto ipIt = std::find_if(begin(ips), end(ips), same);
        auto opIt = std::find_if(begin(ops), end(ops), same);
        if (ipIt != end(ips)) gd.setMaximum(ipIt->getAmount() * ipSpF);
        if (opIt != end(ops)) gd.setMaximum(opIt->getAmount() * opSpF);
    }
    gd.setDemandSlope();
    // Call parameter function.
    fn(gd);
    // Insert good into container with position determined by full id.
    goods.insert(goods.lower_bound(gd.getFullId()), std::move(gd));
}

void Property::reset() {
    // Reset goods to starting amounts.
    double updateTime = Settings::getPropertyUpdateTime();
    double dayLength = Settings::getDayLength();
    // Use up all remaining goods.
    use();
    // Create goods for business inputs.
    for (auto &b : businesses) {
        auto &lastOutput = b.getOutputs().back();
        for (auto &ip : b.getInputs())
            if (ip == lastOutput)
                // Input good is also output, create full amount.
                create(ip.getGoodId(), ip.getAmount());
            else
                // Create only enough for one cycle.
                create(ip.getGoodId(), ip.getAmount() * updateTime / dayLength);
    }
}

std::vector<Good> Property::take(unsigned int gId, double amt) {
    // Take away the given amount of the given good id, proportional among materials. Return transfer goods.
    auto &byGoodId = goods.get<GoodId>();
    auto gdRng = byGoodId.equal_range(gId);
    double total =
        std::accumulate(gdRng.first, gdRng.second, 0., [](double tt, auto &gd) { return tt + gd.getAmount(); });
    std::vector<Good> transfer;
    for (auto gdIt = gdRng.first; gdIt != gdRng.second; ++gdIt) {
        transfer.push_back(Good(gdIt->getFullId(), amt * gdIt->getAmount() / total));
        byGoodId.modify(gdIt, [&tG = transfer.back()](auto &gd) { gd.take(tG); });
    }
    return transfer;
}

void Property::take(Good &gd) {
    // Take the given good from this property.
    auto &byFullId = goods.get<FullId>();
    auto rGdIt = byFullId.find(gd.getFullId());
    if (rGdIt == end(byFullId)) return gd.use();
    byFullId.modify(rGdIt, [&gd](auto &rGd) { rGd.take(gd); });
}

void Property::put(Good &gd) {
    // Put the given good in this property.
    auto fId = gd.getFullId();
    auto &byFullId = goods.get<FullId>();
    auto rGdIt = byFullId.find(fId);
    auto putGood = [&gd](auto &rGd) { rGd.put(gd); };
    if (rGdIt == end(byFullId)) {
        // Good does not exist, copy from source.
        addGood(*source->good(fId), putGood);
    } else
        byFullId.modify(rGdIt, putGood);
}

void Property::use(unsigned int gId, double amt) {
    // Use the given amount of the given good id.
    auto &byGoodId = goods.get<GoodId>();
    auto gdRng = byGoodId.equal_range(gId);
    double total =
        std::accumulate(gdRng.first, gdRng.second, 0., [](double tt, auto &gd) { return tt + gd.getAmount(); });
    if (total == 0) throw std::runtime_error("0 total using good " + std::to_string(gId));
    auto useGood = [amt, total](auto &gd) { gd.use(amt * gd.getAmount() / total); };
    for (; gdRng.first != gdRng.second; ++gdRng.first) byGoodId.modify(gdRng.first, useGood);
}

void Property::use() {
    // Use current amount of all goods.
    for (auto gdIt = begin(goods); gdIt != end(goods); ++gdIt) goods.modify(gdIt, [](auto &gd) { gd.use(); });
}

void Property::create(unsigned int opId, double amt) {
    // Create the given amount of the lowest indexed material of the given good id.
    if (std::isnan(amt)) std::cout << opId;
    auto &byGoodId = goods.get<GoodId>();
    auto opRng = byGoodId.equal_range(opId);
    auto createGood = [amt](auto &gd) { gd.create(amt); };
    if (opRng.first == opRng.second) {
        // Output good doesn't exist, copy from nation.
        auto srRng = source->goods.get<GoodId>().equal_range(opId);
        addGood(*std::min_element(srRng.first, srRng.second), createGood);
    } else
        byGoodId.modify(std::min_element(opRng.first, opRng.second), createGood);
}

void Property::create(unsigned int opId, unsigned int ipId, double amt) {
    // Create the given amount of the first good id based on inputs of the second good id.
    auto &byGoodId = goods.get<GoodId>();
    auto &byMaterialId = goods.get<MaterialId>();
    auto ipRng = byGoodId.equal_range(ipId);
    if (ipRng.first == ipRng.second)
        // Input goods don't exist.
        return;
    double inputTotal =
        std::accumulate(ipRng.first, ipRng.second, 0., [](double tt, auto &gd) { return tt + gd.getAmount(); });
    // Find needed output goods that don't exist.
    for (; ipRng.first != ipRng.second; ++ipRng.first) {
        // Search for good with output good id and input material id.
        auto ipMId = ipRng.first->getMaterialId();
        auto opGMId = boost::make_tuple(opId, ipMId);
        auto opIt = byMaterialId.find(opGMId);
        auto createGood = [cAmt = amt * ipRng.first->getAmount() / inputTotal](auto &gd) { gd.create(cAmt); };
        if (opIt == end(byMaterialId)) {
            // Output good doesn't exist, copy from source.
            addGood(*source->good(opGMId), createGood);
            // Refresh input iterators because insert invalidates them.
            ipRng = byGoodId.equal_range(ipId);
            // Advance first iterator to position of current input material.
            while (ipRng.first->getMaterialId() != ipMId) ++ipRng.first;
        } else
            // Create good.
            byMaterialId.modify(opIt, createGood);
    }
}

void Property::create() {
    // Create maximum amount of all goods.
    for (auto gdIt = begin(goods); gdIt != end(goods); ++gdIt)
        goods.modify(gdIt, [](auto &gd) { gd.create(); });
}

void Property::build(const Business &bsn, double a) {
    // Build the given area of the given business. Check requirements before calling.
    auto bsnsIt = std::lower_bound(begin(businesses), end(businesses), bsn);
    if (bsnsIt == end(businesses) || *bsnsIt != bsn) {
        // Insert new business into properties and set its area.
        auto nwBsns = businesses.insert(bsnsIt, Business(bsn));
        nwBsns->takeRequirements(*this, a);
        nwBsns->setArea(a);
    } else {
        // Business already exists, grow it.
        bsnsIt->takeRequirements(*this, a);
        bsnsIt->changeArea(a);
    }
}

void Property::demolish(const Business &bsn, double a) {
    // Demolish the given area of the given business. Check area before calling.
    auto bsnIt = std::lower_bound(begin(businesses), end(businesses), bsn);
    bsnIt->changeArea(-a);
    bsnIt->reclaim(*this, a);
    if (bsnIt->getArea() == 0) businesses.erase(bsnIt);
}

void Property::update(unsigned int elTm) {
    updateCounter += elTm;
    if (updateCounter > 0) {
        int updateTime = Settings::getPropertyUpdateTime();
        updateTime += updateCounter - updateCounter % updateTime;
        double dayLength = Settings::getDayLength();
        if (maxGoods)
            // Property creates as many goods as possible for testing purposes.
            create();
        // Update goods and run businesses for update time.
        auto updateGood = [updateTime, dayLength](Good &gd) { gd.update(updateTime, dayLength); };
        for (auto gdIt = begin(goods); gdIt != end(goods); ++gdIt) goods.modify(gdIt, updateGood);
        std::unordered_map<unsigned int, Conflict> conflicts;
        for (auto &b : businesses)
            // Start by setting factor to business run time.
            b.setFactor(updateTime / dayLength, *this, conflicts);
        for (auto &b : businesses)
            // Handle conflicts on inputs by reducing factors.
            b.run(*this, conflicts);
        updateCounter -= updateTime;
    }
}

void Property::buttons(Pager &pgr, const SDL_Rect &rt, BoxInfo &bI, Printer &pr,
                       const std::function<std::function<void(MenuButton *)>(const Good &)> &fn) const {
    // Create buttons on the given pager using the given function to generate on click functions.
    pgr.setBounds(rt);
    int m = Settings::getButtonMargin(), dx = (rt.w + m) / Settings::getGoodButtonColumns(),
        dy = (rt.h + m) / Settings::getGoodButtonRows();
    bI.rect = {rt.x, rt.y, dx - m, dy - m};
    std::vector<std::unique_ptr<TextBox>> bxs; // boxes which will go on page
    for (auto &gd : goods) {
        if (true || (gd.getAmount() >= 0.01 && gd.getSplit()) || (gd.getAmount() >= 1.)) {
            bI.onClick = fn(gd);
            bxs.push_back(gd.button(true, bI, pr));
            bI.rect.x += dx;
            if (bI.rect.x + bI.rect.w > rt.x + rt.w) {
                // Go back to left and down one row upon reaching right.
                bI.rect.x = rt.x;
                bI.rect.y += dy;
                if (bI.rect.y + bI.rect.h > rt.y + rt.h) {
                    // Go back to top and flush current page upon reaching bottom.
                    bI.rect.y = rt.y;
                    pgr.addPage(bxs);
                }
            }
        }
    }
    if (bxs.size())
        // Flush remaining boxes to new page.
        pgr.addPage(bxs);
}

void Property::buttons(Pager &pgr, const SDL_Rect &rt, BoxInfo &bI, Printer &pr,
                       const std::function<std::function<void(MenuButton *)>(const Business &)> &fn) const {
    // Create buttons on the given pager using the given function to generate on click functions.
    pgr.setBounds(rt);
    int m = Settings::getButtonMargin(), dx = (rt.w + m) / Settings::getBusinessButtonColumns(),
        dy = (rt.h + m) / Settings::getBusinessButtonRows();
    bI.rect = {rt.x, rt.y, dx - m, dy - m};
    std::vector<std::unique_ptr<TextBox>> bxs; // boxes which will go on page
    for (auto &bsn : businesses) {
        if (true || (bsn.getArea() >= 0.01)) {
            bI.onClick = fn(bsn);
            bxs.push_back(bsn.button(true, bI, pr));
            bI.rect.x += dx;
            if (bI.rect.x + bI.rect.w > rt.x + rt.w) {
                // Go back to left and down one row upon reaching right.
                bI.rect.x = rt.x;
                bI.rect.y += dy;
                if (bI.rect.y + bI.rect.h > rt.y + rt.h) {
                    // Go back to top and flush current page upon reaching bottom.
                    bI.rect.y = rt.y;
                    pgr.addPage(bxs);
                }
            }
        }
    }
    if (bxs.size())
        // Flush remaining boxes to new page.
        pgr.addPage(bxs);
}

void Property::adjustAreas(const std::vector<MenuButton *> &rBs, double d) {
    d *= static_cast<double>(population) / 5000;
    for (auto &b : businesses) {
        bool go = false;
        bool inputMatch = false;
        std::vector<std::string> mMs = {"wool", "hide", "milk", "bowstring", "arrowhead", "cloth"};
        for (auto &rB : rBs) {
            if (rB->getClicked()) {
                std::string rBN = rB->getText()[0];
                for (auto &ip : b.getInputs()) {
                    std::string ipN = ip.getGoodName();
                    if (rBN.substr(0, rBN.find(' ')) == ipN.substr(0, ipN.find(' ')) ||
                        std::find(begin(mMs), end(mMs), ipN) != end(mMs)) {
                        inputMatch = true;
                        break;
                    }
                }
                for (auto &op : b.getOutputs()) {
                    if ((!rB->getId() || op.getGoodId() == goods.get<FullId>().find(rB->getId())->getGoodId()) &&
                        (!b.getKeepMaterial() || inputMatch)) {
                        go = true;
                        break;
                    }
                }
            }
            if (go) break;
        }
        if ((go) && b.getArea() > -d) { b.setArea(b.getArea() + d); }
    }
    setMaximums();
}

void Property::adjustDemand(const std::vector<MenuButton *> &rBs, double d) {
    d /= static_cast<double>(population) / 1000;
    auto &byFullId = goods.get<FullId>();
    for (auto &rB : rBs)
        if (rB->getClicked()) {
            std::string rBN = rB->getText()[0];
            byFullId.modify(byFullId.find(rB->getId()), [d](auto &gd) { gd.adjustDemand(d); });
        }
}

void Property::saveFrequencies(std::string &u) const {
    for (auto &b : businesses) { b.saveFrequency(population, u); }
}

void Property::saveDemand(std::string &u) const {
    for (auto &gd : goods) gd.saveDemand(population, u);
}
