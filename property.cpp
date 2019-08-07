#include "property.hpp"

Property::Property(const Save::Property *ppt, const Property &other) : id(ppt->id()) {
    auto ldGds = ppt->goods();
    for (auto lGI = ldGds->begin(); lGI != ldGds->end(); ++lGI) goods.push_back(Good(*lGI));
    for (size_t i = 0; i < goods.size(); ++i) goods[i].setImage(other.goods[i].getImage());
    auto ldBsns = ppt->businesses();
    for (auto lBI = ldBsns->begin(); lBI != ldBsns->end(); ++lBI) businesses.push_back(Business(*lBI));
    mapGoods();
}

flatbuffers::Offset<Save::Property> Property::save(flatbuffers::FlatBufferBuilder &b) const {
    auto svGoods =
        b.CreateVector<flatbuffers::Offset<Save::Good>>(goods.size(), [this, &b](size_t i) { return goods[i].save(b); });
    auto svBusinesses = b.CreateVector<flatbuffers::Offset<Save::Business>>(
        businesses.size(), [this, &b](size_t i) { return businesses[i].save(b); });
    return Save::CreateProperty(b, id, svGoods, svBusinesses);
}

double Property::amount(unsigned int gId) const {
    auto gdRng = byGoodId.equal_range(gId);
    return std::accumulate(gdRng.first, gdRng.second, 0., [](double t, auto g) { return t + g.second->getAmount(); });
}

double Property::maximum(unsigned int gId) const {
    auto gdRng = byGoodId.equal_range(gId);
    return std::accumulate(gdRng.first, gdRng.second, 0., [](double t, auto g) { return t + g.second->getMaximum(); });
}

void Property::mapGoods() {
    for (auto &gd : goods) byGoodId.emplace(gd.getGoodId(), &gd);
}

void Property::setConsumption(const std::vector<std::array<double, 3>> &gdsCnsptn) {
    for (size_t i = 0; i < goods.size(); ++i) goods[i].setConsumption(gdsCnsptn[i]);
}

void Property::setFrequencies(const std::vector<double> &frqcs) {
    for (size_t i = 0; i < frqcs.size(); ++i) businesses[i].setFrequency(frqcs[i]);
}

void Property::setMaximums() {
    // Set maximum good amounts for given businesses.
    for (auto &g : goods) g.setMaximum();
    // Set good maximums for businesses.
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
            for (auto gdIt = gdRng.first; gdIt != gdRng.second; ++gdIt) gdIt->second->setMaximum(max);
        }
        for (auto &op : ops) {
            auto gdRng = byGoodId.equal_range(op.getGoodId());
            max = op.getAmount() * Settings::getOutputSpaceFactor();
            for (auto gdIt = gdRng.first; gdIt != gdRng.second; ++gdIt) gdIt->second->setMaximum(max);
        }
    }
}

void Property::reset() {
    // Reset goods to starting amounts.
    for (auto &gd : goods) gd.use();
    for (auto &b : businesses)
        for (auto &ip : b.getInputs())
            if (ip == b.getOutputs().back())
                // Input good is also output, create full amount.
                create(ip.getGoodId(), ip.getAmount());
            else
                // Create only enough for one cycle.
                create(ip.getGoodId(), ip.getAmount() * Settings::getBusinessRunTime() /
                                           static_cast<double>(Settings::getDayLength() * kDaysPerYear));
}

void Property::scale(bool ctl, unsigned long ppl, unsigned int tT) {
    for (auto &gd : goods) { gd.scaleConsumption(ppl); }
    std::function<bool(const Business &b)> fn;
    if (ctl)
        fn = [](auto &b) { return b.getFrequency() == 0; };
    else
        fn = [](auto &b) { return b.getRequireCoast() || b.getFrequency() == 0; };
    businesses.erase(std::remove_if(begin(businesses), end(businesses), fn), end(businesses));
    for (auto &bsn : businesses) bsn.scale(ppl, tT);
    setMaximums();
    reset();
}

std::vector<Good> Property::take(unsigned int gId, double amt) {
    auto gdRng = byGoodId.equal_range(gId);
    double total =
        std::accumulate(gdRng.first, gdRng.second, 0, [](double t, auto g) { return t + g.second->getAmount(); });
    std::vector<Good> transfer;
    std::for_each(gdRng.first, gdRng.second, [amt, total, &transfer](auto g) {
        transfer.push_back(Good(g.second->getFullId(), amt * g.second->getAmount() / total));
        g.second->take(transfer.back());
    });
    return transfer;
}

void Property::use(unsigned int gId, double amt) {
    auto gdRng = byGoodId.equal_range(gId);
    double total =
        std::accumulate(gdRng.first, gdRng.second, 0, [](double t, auto g) { return t + g.second->getAmount(); });
    std::for_each(gdRng.first, gdRng.second, [amt, total](auto g) { g.second->use(amt / total); });
}

void Property::create(unsigned int gId, double amt) {
    // Create the given amount of the lowest indexed material of the given good id.
    auto gdRng = byGoodId.equal_range(gId);
    std::min_element(gdRng.first, gdRng.second, [](auto a, auto b) {
        return a.second->getMaterialId() < b.second->getMaterialId();
    })->second->create(amt);
}

void Property::create(unsigned int gId, unsigned int iId, double amt) {
    // Create the given amount of the second good id based on inputs of the first good id.
    auto gdRng = byGoodId.equal_range(gId);
    auto ipRng = byGoodId.equal_range(iId);
    double inputTotal =
        std::accumulate(ipRng.first, ipRng.second, 0, [](double tA, auto g) { return tA + g.second->getAmount(); });
    for (auto gdIt = gdRng.first, ipIt = ipRng.first; gdIt != gdRng.second && ipIt != ipRng.second; ++gdIt, ++ipIt)
        gdIt->second->create(amt * ipIt->second->getAmount() / inputTotal);
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
    for (auto &gd : goods) gd.update(elTm);
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
                    if ((!rB->getId() || op.getGoodId() == getGoodId(rB->getId())) && (!b.getKeepMaterial() || inputMatch)) {
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
    for (auto &rB : rBs)
        if (rB->getClicked()) {
            std::string rBN = rB->getText()[0];
            goods[rB->getId()].adjustDemand(d);
        }
}

void Property::saveFrequencies(unsigned long ppl, std::string &u) const {
    for (auto &b : businesses) { b.saveFrequency(ppl, u); }
}

void Property::saveDemand(unsigned long ppl, std::string &u) const {
    for (auto &gd : goods) gd.saveDemand(ppl, u);
}
