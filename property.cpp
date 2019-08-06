#include "property.hpp"

Property::Property(const Save::Property *ppt, const Property &other) id(ppt->id()) {
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

void Property::setConsumption(const std::vector<std::array<double, 3>> &gdsCnsptn) {
    for (size_t i = 0; i < goods.size(); ++i) goods[i].setConsumption(gdsCnsptn[i]);
}

void Property::setFrequencies(const std::vector<double> &frqcs);
{
    for (size_t i = 0; i < fqs.size(); ++i) businesses[i].setFrequency(fqs[i]);
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

void Property::setAreas(bool ctl, unsigned long ppl, unsigned int tT,
                        const std::map<std::pair<unsigned int, unsigned int>, double> &fFs) {
    for (auto &bsn : businesses) {
        double fq = bsn.getFrequency();
        if (fq != 0 && (ctl || !bsn.getRequireCoast())) {
            double fF = 1;
            auto tTBI = std::make_pair(tT, bsn.getId());
            auto fFI = fFs.lower_bound(tTBI);
            if (fFI != end(fFs) && fFI->first == tTBI) fF = fFI->second;
            bsn.setArea(static_cast<double>(ppl) * fq * fF);
        }
    }
}

std::vector<Good> Property::take(unsigned int gId, double amt) {
    auto gdRng = byGoodId.equal_range(gId);
    double total =
        std::accumulate(gdRng.first, gdRng.second, 0, [](double t, auto g) { return t + g.second->getAmount(); });
    std::vector<Good> transfer;
    std::for_each(gdRng.first, gdRng.second, [amt, total, &transfer](auto g) {
        transfer.push_back(Good(g->getFullId(), amt * g->getAmount() / total));
        g->take(transfer.back());
    });
}

void Property::use(unsigned int gId, double amt) {
    auto gdRng = byGoodId.equal_range(gId);
    double total =
        std::accumulate(gdRng.first, gdRng.second, 0, [](double t, auto g) { return t + g.second->getAmount(); });
    std::for_each(gdRng.first, gdRng.second, [amt, total](Good *g) { g->use(amt / total); });
}

void Property::create(unsigned int gId, double amt) {
    // Create the given amount of the lowest indexed material of the given good id.
    auto gdRng = byGoodId.equal_range(gId);
    std::min_element(gdRng.first, gdRng.second, [](const Good *a, const Good *b) {
        return a->getMaterialId() < b->getMaterialId();
    })->second->create(amt);
}

void Property::create(unsigned int iId, unsigned int oId, double amt) {
    // Create the given amount of the second good id based on inputs of the first good id.
    auto ipRng = byGoodId.equal_range(iId);
    auto opRng = byGoodId.equal_range(oId);
    double inputTotal =
        std::accumulate(ipRng.first, ipRng.second, 0, [](double tA, auto g) { return tA + g.second->getAmount(); });
    for (auto ipIt = ipRng.first, opIt = opRng.first; ipIt != ipRng.second && opIt != opRng.second; ++ipIt, ++opIt)
        opIt->second->create(amt * ipIt->second->getAmount() / inputTotal);
}

void Property::update(unsigned int elTm, std::vector<Business> &businesses) {
    // Update goods and run businesses for given elapsed time.
    for (auto &gd : goods) gd.update(elTm);
    std::vector<unsigned int> conflicts(goods.size(), 0); // number of businesses that need more of each good than we have
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

void Property::reset() {
    // Reset goods to starting amounts.
    for (auto &gd : goods) gd.use();
    for (auto &b : businesses)
        for (auto &ip : b.getInputs())
            if (ip == b.getOutputs().back())
                // Input good is also output, create full amount.
                property.create(ip.getGoodId(), ip.getAmount());
            else
                // Create only enough for one cycle.
                property.create(ip.getGoodId(), ip.getAmount() * Settings::getBusinessRunTime() /
                                                    static_cast<double>(Settings::getDayLength() * kDaysPerYear));
}

void Property::goodButtons(Pager &pgr, const SDL_Rect &rt, BoxInfo &bI, Printer &pr,
                       const std::function<std::function<void(MenuButton *)>(const Good &)> &fn) {
    // Create buttons on the given pager for the given set of goods using the given function to generate on click functions.
    pgr.setBounds(rt);
    int m = Settings::getButtonMargin(), dx = (rt.w + m) / Settings::getGoodButtonColumns(),
        dy = (rt.h + m) / Settings::getGoodButtonRows();
    bI.rect = {rt.x, rt.y, dx - m, dy - m};
    std::vector<std::unique_ptr<TextBox>> bxs; // boxes which will go on page
    for (auto &g : goods) {
        if (true || (g.getAmount() >= 0.01 && g.getSplit()) || (g.getAmount() >= 1.)) {
            bI.onClick = fn(g);
            bxs.push_back(g.button(true, bI, pr));
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
