#include "property.hpp"

Property::Property(const Save::Property *ppt, const Town *tn) : town(tn) {
    auto ldGds = ppt->goods();
    for (auto lGI = ldGds->begin(); lGI != ldGds->end(); ++lGI) goods.push_back(Good(*lGI));
    for (auto gdIt = begin(goods); gdIt != end(goods); ++gdIt)
        goods.modify(gdIt, [srGd = src.good(gdIt->getFullId())](auto &gd) { gd.setImage(srGd.getImage()); });
    auto ldBsns = ppt->businesses();
    for (auto lBI = ldBsns->begin(); lBI != ldBsns->end(); ++lBI) businesses.push_back(Business(*lBI));
}

flatbuffers::Offset<Save::Property> Property::save(flatbuffers::FlatBufferBuilder &b) const {
    std::vector<Good> gds(begin(goods), end(goods));
    auto svGoods = b.CreateVector<flatbuffers::Offset<Save::Good>>(
        goods.size(), [&b, &gds](size_t i) { return gds[i].save(b); });
    auto svBusinesses = b.CreateVector<flatbuffers::Offset<Save::Business>>(
        businesses.size(), [this, &b](size_t i) { return businesses[i].save(b); });
    return Save::CreateProperty(b, id, svGoods, svBusinesses);
}

bool Property::hasGood(unsigned int fId) const {
    auto &byFullId = goods.get<FullId>();
    return byFullId.find(fId) != end(byFullId);
}

void Property::queryGoods(const std::function<void(const Good &gd)> &fn) const {
    for (auto &gd : goods) { fn(gd); }
}

double Property::amount(unsigned int gId) const {
    auto gdRng = goods.get<GoodId>().equal_range(gId);
    return std::accumulate(gdRng.first, gdRng.second, 0., [](double tt, auto &gd) { return tt + gd.getAmount(); });
}

double Property::maximum(unsigned int gId) const {
    auto gdRng = goods.get<GoodId>().equal_range(gId);
    return std::accumulate(gdRng.first, gdRng.second, 0., [](double tt, auto &gd) { return tt + gd.getMaximum(); });
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
    for (auto gdIt = begin(goods); gdIt != end(goods); ++gdIt) goods.modify(gdIt, [](auto &gd) { gd.setMaximum(); });
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
}

void Property::reset() {
    // Reset goods to starting amounts.
    for (auto gdIt = begin(goods); gdIt != end(goods); ++gdIt) goods.modify(gdIt, [](auto &gd) { gd.use(); });
    for (auto &b : businesses) {
        for (auto &ip : b.getInputs())
            if (ip == b.getOutputs().back())
                // Input good is also output, create full amount.
                create(ip.getGoodId(), ip.getAmount());
            else
                // Create only enough for head start.
                create(ip.getGoodId(), ip.getAmount() * Settings::getBusinessHeadStart() / (Settings::getDayLength() * kDaysPerYear));
    }
}

void Property::scale(bool ctl, unsigned long ppl, unsigned int tT) {
    // Copy businesses from source that exist in this town.
    auto &source = town->getNation()->getProperty();
    std::copy_if(begin(source.businesses), end(source.businesses), std::back_inserter(businesses),
                 [ctl](auto &bsn) { return bsn.getFrequency() > 0 && (!bsn.getRequireCoast() || ctl); });
    // Scale business areas according to town population and type.
    for (auto &bsn : businesses) bsn.scale(ppl, tT);
    // Create starting input goods.
    reset();
    // Scale good consumption data.
    for (auto gdIt = begin(goods); gdIt != end(goods); ++gdIt) goods.modify(gdIt, [ppl](auto &gd) { gd.scale(ppl); });
    // Set max amounts for goods.
    setMaximums();
}

std::vector<Good> Property::take(unsigned int gId, double amt) {
    // Take away the given amount of the given good id. Return transfer goods.
    auto &byGoodId = goods.get<GoodId>();
    auto gdRng = byGoodId.equal_range(gId);
    double total = std::accumulate(gdRng.first, gdRng.second, 0, [](double tt, auto &gd) { return tt + gd.getAmount(); });
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
    if (rGdIt == end(byFullId)) {
        // Good does not exist, copy from source.
        auto &source = town->getNation()->getProperty();
        goods.push_back(source.good(fId));
        rGdIt = byFullId.find(fId);
    }
    byFullId.modify(rGdIt, [&gd](auto &rGd) { rGd.put(gd); });
}

void Property::use(unsigned int gId, double amt) {
    // Use the given amount of the given good id.
    auto &byGoodId = goods.get<GoodId>();
    auto gdRng = byGoodId.equal_range(gId);
    double total = std::accumulate(gdRng.first, gdRng.second, 0, [](double t, auto &gd) { return t + gd.getAmount(); });
    for (; gdRng.first != gdRng.second; ++gdRng.first)
        byGoodId.modify(gdRng.first, [amt, total](auto &gd) { gd.use(amt * gd.getAmount() / total); });
}

void Property::use() {
    // Use current amount of all goods.
    for (auto gdIt = begin(goods); gdIt != end(goods); ++gdIt) goods.modify(gdIt, [](auto &gd) { gd.use(); });
}

void Property::create(unsigned int opId, double amt) {
    // Create the given amount of the lowest indexed material of the given good id.
    auto &byGoodId = goods.get<GoodId>();
    auto opRng = byGoodId.equal_range(opId);
    auto lowest = [](auto &gA, auto &gB) { return gA.getMaterialId() < gB.getMaterialId(); };
    if (opRng.first == opRng.second) {
        // Output good doesn't exist, copy from nation.
        auto &source = town->getNation()->getProperty();
        auto srRng = source.goods.get<GoodId>().equal_range(opId);
        goods.push_back(*std::min_element(srRng.first, srRng.second, lowest));
        opRng = goods.get<GoodId>().equal_range(opId);
    }
    byGoodId.modify(std::min_element(opRng.first, opRng.second, lowest), [amt](auto &gd) { gd.create(amt); });
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
    // Store input material ids in vector because iterators will invalidate on emplace.
    std::vector<unsigned int> ipMIds;
    for (; ipRng.first != ipRng.second; ++ipRng.first)
        ipMIds.push_back(ipRng.first->getMaterialId());
    // Find needed output goods that don't exist.
    for (auto &ipMId : ipMIds) {
        // Search for good with output good id and input material id.
        auto opGMId = boost::make_tuple(opId, ipMId);
        auto opIt = byMaterialId.find(opGMId);
        if (opIt == end(byMaterialId)) {
            // Output good doesn't exist, copy from source.
            opIt = byMaterialId.insert(source.good(opGMId)).first;
            byMaterialId.modify(opIt, [](auto &gd) { gd.setMaximum(); });
        }
        // Create good.
        byMaterialId.modify(opIt, [amount = amt * ipRng.first->getAmount() / inputTotal](auto &gd) { gd.create(amount); });
    }
}

void Property::create() {
    // Create maximum amount of all goods.
    for (auto gdIt = begin(goods); gdIt != end(goods); ++gdIt) goods.modify(gdIt, [](auto &gd) { gd.create(); });
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
    // Update goods and run businesses for given elapsed time.
    for (auto gdIt = begin(goods); gdIt != end(goods); ++gdIt)
        goods.modify(gdIt, [elTm](auto &gd) { gd.update(elTm); });
    auto dayLength = Settings::getDayLength();
    for (auto &b : businesses) {
        // For each business, start by setting factor to business run time.
        b.setFactor(elTm / static_cast<double>(kDaysPerYear * dayLength));
        // Count conflicts of businesses for available goods.
        b.addConflicts(conflicts, *this);
    }
    for (auto &b : businesses) {
        // Handle conflicts by reducing factors.
        b.handleConflicts(conflicts);
        // Run businesses on town's goods.
        b.run(*this);
    }
}

void Property::buttons(Pager &pgr, const SDL_Rect &rt, BoxInfo &bI, Printer &pr,
                       const std::function<std::function<void(MenuButton *)>(const Good &)> &fn) const {
    // Create buttons on the given pager for the given set of goods using the given function to generate on click functions.
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
    // Create buttons on the given pager for the given set of goods using the given function to generate on click functions.
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
    auto &byFullId = goods.get<FullId>();
    for (auto &rB : rBs)
        if (rB->getClicked()) {
            std::string rBN = rB->getText()[0];
            byFullId.modify(byFullId.find(rB->getId()), [d](auto &gd) { gd.adjustDemand(d); });
        }
}

void Property::saveFrequencies(unsigned long ppl, std::string &u) const {
    for (auto &b : businesses) { b.saveFrequency(ppl, u); }
}

void Property::saveDemand(unsigned long ppl, std::string &u) const {
    for (auto &gd : goods) gd.saveDemand(ppl, u);
}
