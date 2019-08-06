#include "inventory.hpp"

void Inventory::setConsumption(const std::vector<std::array<double, 3>> &gdsCnsptn) {
    for (size_t i = 0; i < goods.size(); ++i) goods[i].setConsumption(gdsCnsptn[i]);
}

void Inventory::setMaximum(const std::vector<Business> &bsns) {
    // Set maximum good amounts for given businesses.
    for (auto &g : goods) g.setMaximum();
    // Set good maximums for businesses.
    for (auto &b : bsns) {
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

std::vector<Good> Inventory::take(unsigned int gId, double amt) {
    auto gdRng = byGoodId.equal_range(gId);
    double total =
        std::accumulate(gdRng.first, gdRng.second, 0, [](double t, auto g) { return t + g.second->getAmount(); });
    std::vector<Good> transfer;
    std::for_each(gdRng.first, gdRng.second, [amt, total, &transfer](auto g) {
        transfer.push_back(Good(g->getFullId(), amt * g->getAmount() / total));
        g->take(transfer.back());
    });
}

void Inventory::use(unsigned int gId, double amt) {
    auto gdRng = byGoodId.equal_range(gId);
    double total =
        std::accumulate(gdRng.first, gdRng.second, 0, [](double t, auto g) { return t + g.second->getAmount(); });
    std::for_each(gdRng.first, gdRng.second, [amt, total](Good *g) { g->use(amt / total); });
}

void Inventory::create(unsigned int gId, double amt) {
    // Create the given amount of the lowest indexed material of the given good id.
    auto gdRng = byGoodId.equal_range(gId);
    std::min_element(gdRng.first, gdRng.second, [](const Good *a, const Good *b) {
        return a->getMaterialId() < b->getMaterialId();
    })->second->create(amt);
}

void Inventory::create(unsigned int iId, unsigned int oId, double amt) {
    // Create the given amount of the second good id based on inputs of the first good id.
    auto ipRng = byGoodId.equal_range(iId);
    auto opRng = byGoodId.equal_range(oId);
    double inputTotal =
        std::accumulate(ipRng.first, ipRng.second, 0, [](double tA, auto g) { return tA + g.second->getAmount(); });
    for (auto ipIt = ipRng.first, opIt = opRng.first; ipIt != ipRng.second && opIt != opRng.second; ++ipIt, ++opIt)
        opIt->second->create(amt * ipIt->second->getAmount() / inputTotal);
}

void Inventory::update(unsigned int elTm, std::vector<Business> &bsns) {
    // Update goods and run businesses for given elapsed time.
    update(elTm);
    std::vector<unsigned int> conflicts(goods.size(), 0); // number of businesses that need more of each good than we have
    auto dayLength = Settings::getDayLength();
    for (auto &b : bsns) {
        // For each business, start by setting factor to business run time.
        b.setFactor(elTm / static_cast<double>(kDaysPerYear * dayLength));
        // Count conflicts of businesses for available goods.
        b.addConflicts(conflicts, *this);
    }
    for (auto &b : bsns) {
        // Handle conflicts by reducing factors.
        b.handleConflicts(conflicts);
        // Run businesses on town's goods.
        b.run(*this);
    }
}

void Inventory::buttons(Pager &pgr, const SDL_Rect &rt, BoxInfo bI, Printer &pr,
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

void Inventory::adjustDemand(const std::vector<MenuButton *> &rBs, double d) {
    for (auto &rB : rBs)
        if (rB->getClicked()) {
            std::string rBN = rB->getText()[0];
            goods[rB->getId()].adjustDemand(d);
        }
}