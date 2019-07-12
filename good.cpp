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

Good::Good(unsigned int i, const std::string &n, double a, double p, double c, const std::string &m)
    : id(i), name(n), amount(a), perish(p), carry(c), measure(m), split(not m.empty()) {}

Good::Good(unsigned int i, const std::string &n, double p, double c, const std::string &m) : Good(i, n, 0, p, c, m) {}

Good::Good(unsigned int i, const std::string &n, double a, const std::string &m) : Good(i, n, a, 0, 0, m) {}

Good::Good(unsigned int i, const std::string &n, double a) : Good(i, n, a, 0, 0, "") {}

Good::Good(unsigned int i, const std::string &n) : Good(i, n, 0) {}

Good::Good(unsigned int i, double a) : Good(i, "", a) {}

Good::Good(unsigned int i) : Good(i, "") {}

Good::Good(const Save::Good *g)
    : id(static_cast<unsigned int>(g->id())), name(g->name()->str()), amount(g->amount()), perish(g->perish()),
      carry(g->carry()), measure(g->measure()->str()), split(not measure.empty()), shoots(g->shoots()) {
    auto lMaterials = g->materials();
    for (auto lMI = lMaterials->begin(); lMI != lMaterials->end(); ++lMI)
        materials.push_back(Material(*lMI));
}

const Material &Good::getMaterial(const Material &m) const {
    return *std::lower_bound(materials.begin(), materials.end(), m);
}

double Good::getConsumption() const {
    double c = 0;
    for (auto &m : materials)
        c += m.getConsumption();
    return c;
}

std::string Good::logEntry() const {
    std::string lT = std::to_string(amount);
    dropTrail(lT, split ? 3 : 0);
    if (split) {
        lT += " " + measure;
        if (amount != 1)
            lT += "s";
        lT += " of";
    }
    lT += " ";
    if (id != materials.front().getId())
        lT += materials.front().getName() + " ";
    lT += name;
    if (not split and amount != 1)
        lT += "s";
    return lT;
}

void Good::setAmount(double a) {
    amount = a;
    for (auto &m : materials)
        m.setAmount(amount / static_cast<double>(materials.size()));
}

void Good::addMaterial(Material m) { materials.push_back(m); }

void Good::setCombatStats(const std::unordered_map<unsigned int, std::vector<CombatStat>> &cSs) {
    for (auto &m : materials) {
        auto it = cSs.find(m.getId());
        if (it == cSs.end())
            continue;
        m.setCombatStats(it->second);
    }
}

void Good::assignConsumption(const std::unordered_map<unsigned int, std::array<double, 3>> &cs) {
    for (auto &m : materials) {
        auto it = cs.find(m.getId());
        if (it == cs.end())
            continue;
        m.assignConsumption(it->second);
    }
}

void Good::assignConsumption(unsigned long p) {
    for (auto &m : materials)
        m.assignConsumption(p);
}

void Good::removeExcess() {
    // Removes materials in excess of max
    if (max > 0 and amount > max) {
        for (auto &m : materials) {
            m.use((amount - max) * m.getAmount() / amount);
        }
        amount = max;
    }
}

void Good::take(Good &g) {
    // Takes the given good from this good. Stores perish counters for transfer in parameter.
    for (auto &gM : g.materials) {
        // Find parameter material in this good's materials.
        auto it = std::lower_bound(materials.begin(), materials.end(), gM);
        if (it == materials.end() or *it != gM) // Material was not found, so take
            // its amount from overall transfer.
            g.amount -= gM.getAmount();
        else
            it->take(gM);
    }
    amount -= g.amount;
}

void Good::use(double a) {
    // Uses up the given amount of this good. Each material is used proportionally.
    if (a > 0. and amount >= a) {
        for (auto &m : materials)
            m.use(a * m.getAmount() / amount);
        amount -= a;
    } else if (a > 0.) {
        for (auto &m : materials)
            m.use(m.getAmount());
        amount = 0.;
    }
}

void Good::put(Good &g) {
    // Puts the given good in this good. Transfers perish counters.
    for (auto &gM : g.materials) {
        auto it = std::lower_bound(materials.begin(), materials.end(), gM);
        if (it == materials.end() or *it != gM)
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
        if (it == mAs.end())
            continue;
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
    if (it == materials.end() or *it != m)
        return;
    it->create(a);
    amount += a;
    removeExcess();
}

void Good::consume(unsigned int e) {
    // Remove consumed goods and perished goods over elapsed time.
    for (auto &m : materials) {
        amount -= m.consume(e);
        if (perish != 0.)
            amount -= m.perish(e, perish);
        if (amount < 0)
            amount = 0;
        // Keep demand slopes from being too low or too high
        m.fixDemand(max);
    }
}

std::unique_ptr<MenuButton> Good::button(bool aS, const Material &mtr, const SDL_Rect &rt, const SDL_Color &fgr,
                                         const SDL_Color &bgr, int b, int r, int fS, Printer &pr,
                                         const std::function<void()> &fn) const {
    auto &oMtr = getMaterial(mtr);
    std::vector<std::string> tx = {oMtr.getName()};
    if (tx.front() != name)
        tx.front() += ' ' + name;
    // Find image in game data.
    SDL_Surface *img = oMtr.getImage();
    if (aS) {
        // Button will have amount shown.
        std::string amountText = std::to_string(oMtr.getAmount());
        dropTrail(amountText, split ? 3 : 0);
        tx.push_back(std::move(amountText));
        return std::make_unique<MenuButton>(rt, tx, fgr, bgr, id, false, b, r, fS, img, pr, fn);
    }
    // Button does not show amount.
    return std::make_unique<MenuButton>(rt, tx, fgr, bgr, id, false, b, r, fS, img, pr, fn);
}

void Good::adjustDemand(std::string rBN, double d) {
    for (auto &m : materials) {
        std::string mN = m.getName();
        if (rBN == mN.substr(0, mN.find(' '))) {
            m.adjustDemand(d);
        }
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
