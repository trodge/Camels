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

#include "good.hpp"

Good::Good(unsigned int i, const std::string &n, double a, double p, double c, const std::string &m, unsigned int s)
    : id(i), name(n), amount(a), perish(p), carry(c), measure(m), split(!m.empty()), shoots(s) {}

Good::Good(unsigned int i, const std::string &n, double p, double c, const std::string &m, unsigned int s)
    : Good(i, n, 0., p, c, m, s) {}

Good::Good(unsigned int i, const std::string &n, double a, double p, double c, const std::string &m)
    : Good(i, n, a, p, c, m, 0) {}

Good::Good(unsigned int i, const std::string &n, double p, double c, const std::string &m) : Good(i, n, 0., p, c, m) {}

Good::Good(unsigned int i, const std::string &n, double a, const std::string &m) : Good(i, n, a, 0., 0., m) {}

Good::Good(unsigned int i, const std::string &n, double a) : Good(i, n, a, "") {}

Good::Good(unsigned int i, const std::string &n) : Good(i, n, 0) {}

Good::Good(unsigned int i, double a) : Good(i, "", a) {}

Good::Good(unsigned int i) : Good(i, "") {}

Good::Good(const Save::Good *g)
    : id(static_cast<unsigned int>(g->id())), name(g->name()->str()), amount(g->amount()), perish(g->perish()),
      carry(g->carry()), measure(g->measure()->str()), split(!measure.empty()), shoots(g->shoots()) {
    auto lMaterials = g->materials();
    for (auto lMI = lMaterials->begin(); lMI != lMaterials->end(); ++lMI) materials.push_back(Material(*lMI));
}

std::string Good::getFullName(const Material &m) const { return id == m.getId() ? name : m.getName() + " " + name; }

const Material &Good::getMaterial(const Material &m) const {
    return *std::lower_bound(materials.begin(), materials.end(), m);
}

const Material &Good::getMaterial(const std::string &mNm) const {
    // Return material such that first word of material name matches first word of parameter.
    return *std::find_if(materials.begin(), materials.end(), [mNm](const Material &m) {
        const std::string &mN = m.getName(); // material name
        return mN.substr(0, mN.find(' ')) == mNm.substr(0, mNm.find(' '));
    }); // iterator to the material on bx
}

double Good::getConsumption() const {
    double c = 0;
    for (auto &m : materials) c += m.getConsumption();
    return c;
}

std::string Good::businessText() const {
    std::string bsnTx = std::to_string(amount);
    dropTrail(bsnTx, split ? 3 : 0);
    if (split) {
        // Goods that split must be measured.
        bsnTx += " " + measure;
        if (amount != 1.)
            // Pluralize.
            bsnTx += "s";
    }
    if (split || amount == 1. || name == "sheep")
        bsnTx = name + ": " + bsnTx;
    else
        bsnTx = name + "s: " + bsnTx;
    return bsnTx;
}

std::string Good::logText() const {
    std::string lTx = std::to_string(amount);
    dropTrail(lTx, split ? 3 : 0);
    if (split) {
        // Goods that split must be measured.
        lTx += " " + measure;
        if (amount != 1.)
            // Pluralize.
            lTx += "s";
        lTx += " of";
    }
    lTx += " ";
    if (id != materials.front().getId()) lTx += materials.front().getName() + " ";
    lTx += name;
    if (!split && amount != 1. && name != "sheep")
        // Pluralize.
        lTx += "s";
    return lTx;
}

void Good::setAmount(double a) {
    // Set amount to given amount proportionally distributed among materials, if present.
    for (auto &m : materials) m.setAmount(a * m.getAmount() / amount);
    amount = a;
}

void Good::addMaterial(const Material &m) { materials.push_back(m); }

void Good::addMaterial(const Good &g) { materials.push_back(Material(g.id, g.name)); }

void Good::setCombatStats(const std::unordered_map<unsigned int, std::vector<CombatStat>> &cSs) {
    for (auto &m : materials) {
        auto it = cSs.find(m.getId());
        if (it == cSs.end()) continue;
        m.setCombatStats(it->second);
    }
}

void Good::setConsumptions(const std::vector<std::array<double, 3>> &cs) {
    // Takes a vector of consumption rates, demand slopes, and demand intercepts for each material of this good.
    for (size_t i = 0; i < cs.size(); ++i) materials[i].setConsumption(cs[i]);
}

void Good::scaleConsumptions(unsigned long p) {
    for (auto &m : materials) m.scaleConsumption(p);
}

void Good::removeExcess() {
    // Removes materials in excess of max
    if (max > 0 && amount > max) {
        for (auto &m : materials) { m.use((amount - max) * m.getAmount() / amount); }
        amount = max;
    }
}

void Good::take(Good &g) {
    // Takes the given good from this good. Stores perish counters for transfer in parameter.
    for (auto &gM : g.materials) {
        // Find parameter material in this good's materials.
        auto it = std::lower_bound(materials.begin(), materials.end(), gM);
        if (it == materials.end() || *it != gM)
            // Material was not found, so take its amount from overall transfer.
            g.amount -= gM.getAmount();
        else
            it->take(gM);
    }
    amount -= g.amount;
}

void Good::use(double a) {
    // Uses up the given amount of this good. Each material is used proportionally.
    if (a > 0. && amount >= a) {
        for (auto &m : materials) m.use(a * m.getAmount() / amount);
        amount -= a;
    } else if (a > 0.) {
        for (auto &m : materials) m.use(m.getAmount());
        amount = 0.;
    }
}

void Good::put(Good &g) {
    // Puts the given good in this good. Transfers perish counters.
    for (auto &gM : g.materials) {
        auto it = std::lower_bound(materials.begin(), materials.end(), gM);
        if (it == materials.end() || *it != gM)
            g.amount += gM.getAmount();
        else
            it->put(gM);
    }
    amount += g.amount;
}

void Good::create(const std::unordered_map<unsigned int, double> &mAs) {
    // Newly creates the given amount of each material.
    double a = 0;
    for (auto &m : materials) {
        auto it = mAs.find(m.getId());
        if (it == mAs.end()) continue;
        double mA = it->second;
        m.create(mA);
        a += mA;
    }
    amount += a;
    removeExcess();
}

void Good::create(double a) {
    // Newly creates the given amount of this good's self named material
    Material m(id);
    auto it = std::lower_bound(materials.begin(), materials.end(), m);
    if (it == materials.end() || *it != m) return;
    it->create(a);
    amount += a;
    removeExcess();
}

void Good::consume(unsigned int e) {
    // Remove consumed goods and perished goods over elapsed time.
    for (auto &m : materials) {
        amount -= m.consume(e);
        if (perish != 0.) amount -= m.perish(e, perish);
        if (amount < 0) amount = 0;
        // Keep demand slopes from being too low or too high
        m.fixDemand(max);
    }
}

std::unique_ptr<MenuButton> Good::button(bool aS, const Material &mtr, BoxInfo bI, Printer &pr) const {
    auto &oMtr = getMaterial(mtr);
    bI.text = {getFullName(mtr)};
    bI.id = id;
    // Find image in game data.
    SDL_Surface *img = oMtr.getImage();
    if (img) bI.images = {{img, {2 * bI.border, 2 * bI.border, img->w, img->h}}};
    if (aS) {
        // Button will have amount shown.
        std::string amountText = std::to_string(oMtr.getAmount());
        dropTrail(amountText, split ? 3 : 0);
        bI.text.push_back(std::move(amountText));
        return std::make_unique<MenuButton>(bI, pr);
    }
    // Button does not show amount.
    return std::make_unique<MenuButton>(bI, pr);
}

void Good::adjustDemand(std::string rBN, double d) {
    for (auto &m : materials) {
        std::string mN = m.getName();
        if (rBN == mN.substr(0, mN.find(' '))) { m.adjustDemand(d); }
    }
}

void Good::saveDemand(unsigned long p, std::string &u) const {
    for (auto &m : materials) {
        u.append(" WHEN good_id = ");
        u.append(std::to_string(id));
        m.saveDemand(p, u);
    }
}

flatbuffers::Offset<Save::Good> Good::save(flatbuffers::FlatBufferBuilder &b) const {
    auto sName = b.CreateString(name);
    auto sMaterials = b.CreateVector<flatbuffers::Offset<Save::Material>>(
        materials.size(), [this, &b](size_t i) { return materials[i].save(b); });
    auto sMeasure = b.CreateString(measure);
    return Save::CreateGood(b, id, sName, amount, sMaterials, perish, carry, sMeasure, shoots);
}
