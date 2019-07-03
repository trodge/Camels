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

#include "material.h"

Material::Material(unsigned int i, const std::string &n, double a, double c, double dS, double dI)
    : id(i), name(n), amount(a), consumption(c), demandSlope(dS), demandIntercept(dI),
      minPrice(dI / Settings::getMinPriceDivisor()), lastAmount(a) {}

Material::Material(unsigned int i, const std::string &n, double c, double dS, double dI) : Material(i, n, 0, c, dS, dI) {}

Material::Material(unsigned int i, const std::string &n, double a) : Material(i, n, a, 0, 0, 0) {}

Material::Material(unsigned int i, const std::string &n) : Material(i, n, 0) {}

Material::Material(unsigned int i, double a) : Material(i, "", a) {}

Material::Material(unsigned int i) : Material(i, "") {}

Material::Material(const Save::Material *m)
    : id(static_cast<unsigned int>(m->id())), name(m->name()->str()), amount(m->amount()), consumption(m->consumption()),
      demandSlope(m->demandSlope()), demandIntercept(m->demandIntercept()), minPrice(demandIntercept / 63) {
    auto lPerishCounters = m->perishCounters();
    for (auto lMI = lPerishCounters->begin(); lMI != lPerishCounters->end(); ++lMI)
        perishCounters.push_back({(*lMI)->time(), (*lMI)->amount()});
    auto lCombatStats = m->combatStats();
    for (auto lCS = lCombatStats->begin(); lCS != lCombatStats->end(); ++lCS)
        combatStats.push_back({(*lCS)->statId(),
                               (*lCS)->partId(),
                               (*lCS)->attack(),
                               (*lCS)->type(),
                               (*lCS)->speed(),
                               {{(*lCS)->bashDefense(), (*lCS)->cutDefense(), (*lCS)->stabDefense()}}});
}

double Material::getPrice(double q) const {
    // Get the price offered when selling the given quantity
    double p = (demandIntercept - demandSlope * (amount + q / 2)) * q;
    if (p < minPrice * q)
        p = minPrice * q;
    return p;
}

double Material::getPrice() const {
    // Get the current price of this material
    double p = demandIntercept - demandSlope * amount;
    if (p < minPrice)
        p = minPrice;
    return p;
}

double Material::getCost(double q) const {
    // Get the cost to buy the given quantity
    double c = (demandIntercept - demandSlope * (amount - q / 2)) * q;
    if (c < minPrice * q)
        c = minPrice * q;
    return c;
}

double Material::getQuantity(double p, double *e) const {
    /* Get quantity of this material available at given price.
     * Second parameter holds excess quantity after amount is used up. */
    double q;
    if (demandSlope != 0.)
        q = amount -
            (demandIntercept - sqrt((demandIntercept - demandSlope * amount) * (demandIntercept - demandSlope * amount) +
                                    demandSlope * p * 2)) /
                demandSlope;
    else if (demandIntercept != 0.)
        q = p / demandIntercept;
    else
        q = 0;
    if (q < 0)
        return 0;
    if (amount > q)
        return q;
    // there's not enough good to sell in the town
    *e = q - amount;
    return amount;
}

double Material::getQuantity(double p) const {
    // Get quantity of this material corresponding to price. Ignore material availability.
    double q;
    if (demandSlope != 0.)
        q = amount -
            (demandIntercept - sqrt((demandIntercept - demandSlope * amount) * (demandIntercept - demandSlope * amount) +
                                    demandSlope * p * 2)) /
                demandSlope;
    else if (demandIntercept != 0.)
        q = p / demandIntercept;
    else
        q = 0;
    if (q < 0)
        return 0;
    return q;
}

double Material::getQuantum(double c) const {
    // Get quantum of this material needed to match given cost.
    double q;
    if (demandSlope != 0.)
        q = amount -
            (demandIntercept - sqrt((demandIntercept - demandSlope * amount) * (demandIntercept - demandSlope * amount) -
                                    demandSlope * c * 2)) /
                demandSlope;
    else if (demandIntercept != 0.)
        q = c / demandIntercept;
    else
        q = 0;
    if (q < 0)
        return 0;
    return q;
}

void Material::assignConsumption(std::array<double, 3> c) {
    // Assign the given three values to consumption, demand slope, and demand intercept.
    consumption = c[0];
    demandSlope = c[1];
    demandIntercept = c[2];
}

void Material::assignConsumption(unsigned long p) {
    // Assign consumption to match given population.
    demandSlope /= static_cast<double>(p);
    consumption *= static_cast<double>(p);
}

void Material::take(Material &m) {
    // Take the given material and keep the perish counters used in t.pCs.
    amount -= m.amount;
    if (amount < 0) {
        m.amount += amount;
        amount = 0;
    }
    double movedAmount = m.getAmount();
    while (movedAmount > 0 and not perishCounters.empty()) {
        // amount is going down
        PerishCounter pC = perishCounters.back();
        perishCounters.pop_back();
        if (pC.amount > movedAmount) {
            // perish counter is enough to make change and stay around
            pC.amount -= movedAmount;
            perishCounters.push_back(pC);
            // taken pc has just the amount taken
            pC.amount = movedAmount;
            m.perishCounters.push_front(pC);
            movedAmount = 0;
        } else {
            // perish counter is used up to make change
            m.perishCounters.push_front(pC);
            movedAmount -= pC.amount;
        }
    }
}

void Material::use(double a) {
    // Use the given amount and discard the perish counters.
    if (a > 0. and amount >= a)
        amount -= a;
    else
        a = 0.;
    while (a > 0. and not perishCounters.empty()) {
        // amount is going down
        PerishCounter pC = perishCounters.back();
        perishCounters.pop_back();
        if (pC.amount > a) {
            // perish counter is enough to make change and stay around
            pC.amount -= a;
            perishCounters.push_back(pC);
            a = 0.;
        } else {
            // perish counter is used up to make change
            a -= pC.amount;
        }
    }
}

void Material::put(Material &m) {
    amount += m.amount;
    auto middle = perishCounters.insert(perishCounters.end(), m.perishCounters.begin(), m.perishCounters.end());
    std::inplace_merge(perishCounters.begin(), middle, perishCounters.end());
}

void Material::create(double a) {
    // Newly create the given amount of this material.
    amount += a;
    if (a > 0)
        perishCounters.push_front({0, a});
}

double Material::perish(unsigned int e, double p) {
    // Perish this material for e milliseconds with maximum shelf life p years. Find the first perish counter that will
    // expire.
    PerishCounter ePC = {int(p * Settings::getDayLength() * kDaysPerYear - e), 0};
    auto expired = std::upper_bound(perishCounters.begin(), perishCounters.end(), ePC);
    // Remove expired amounts from amount and total them.
    double a =
        std::accumulate(expired, perishCounters.end(), 0, [](double d, const PerishCounter &pC) { return d + pC.amount; });
    // Erase expired perish counters.
    perishCounters.erase(expired, perishCounters.end());
    // Add elapsed time to remaining counters.
    for (auto &pC : perishCounters)
        pC.time += e;
    // If too much perished, reduce total by overage.
    if (amount > a) {
        amount -= a;
    } else {
        a = amount;
        amount = 0;
    }
    // Return total amount perished.
    return a;
}

double Material::consume(unsigned int e) {
    lastAmount = amount;
    double c = consumption * e / kDaysPerYear / Settings::getDayLength();
    if (c > amount)
        c = amount;
    if (c > 0.)
        use(c);
    else if (c < 0.)
        create(-c);
    return c;
}

std::unique_ptr<MenuButton> Material::button(bool aS, unsigned int gI, const std::string &gN, bool gS, SDL_Rect rt,
                                             SDL_Color fgr, SDL_Color bgr, int b, int r, int fS, const GameData &gD, std::function<void()> f) const {
    // Create a new button to represent this material.
    auto nameText = name;
    if (nameText != gN)
        nameText += ' ' + gN;
    std::vector<std::string> tx;
    tx.push_back(std::move(nameText));
    // Find image in game data.
    SDL_Surface *img = gD.goodImages[gI].find(id)->second;
    if (aS) {
        // Button will have amount shown.
        std::string amountText = std::to_string(amount);
        dropTrail(&amountText, gS ? 3 : 0);
        tx.push_back(std::move(amountText));
        return std::make_unique<MenuButton>( rt, tx, fgr, bgr, gI, false, b, r, fS, img, f);
    }
    // Button does not show amount.
    return std::make_unique<MenuButton>( rt, tx, fgr, bgr, gI, false, b, r, fS, img, f);
}

void Material::updateButton(bool gS, double oV, int rC, TextBox *b) const {
    // Update amount shown on this material's button
    std::string amountText;
    if (rC and oV > 0.) {
        double quantity = getQuantity(oV / rC * Settings::getTownProfit());
        amountText = std::to_string(std::min(quantity, amount));
    } else
        amountText = std::to_string(amount);
    std::string changeText = std::to_string(amount - lastAmount);
    dropTrail(&changeText, 5);
    dropTrail(&amountText, gS ? 3 : 0);
    b->setText({b->getText()[0], amountText, changeText});
}

void Material::adjustDemand(double d) {
    demandSlope += d * demandIntercept;
    if (demandSlope < 0)
        demandSlope = 0;
}

void Material::fixDemand(double m) {
    // Check if price goes too low when good is at maximum.
    if (demandIntercept - demandSlope * m < minPrice)
        demandSlope = (demandIntercept - minPrice) / m;
}

void Material::saveDemand(unsigned long p, std::string &u) const {
    u.append(" AND material_id = ");
    u.append(std::to_string(id));
    u.append(" THEN ");
    u.append(std::to_string(demandSlope * static_cast<double>(p)));
}

flatbuffers::Offset<Save::Material> Material::save(flatbuffers::FlatBufferBuilder &b) const {
    auto sName = b.CreateString(name);
    auto sPerishCounters =
        b.CreateVectorOfStructs<Save::PerishCounter>(perishCounters.size(), [this](size_t i, Save::PerishCounter *pC) {
            *pC = Save::PerishCounter(perishCounters[i].time, perishCounters[i].amount);
        });
    auto sCombatStats =
        b.CreateVectorOfStructs<Save::CombatStat>(combatStats.size(), [this](size_t i, Save::CombatStat *cS) {
            *cS = Save::CombatStat(combatStats[i].statId, combatStats[i].partId, combatStats[i].attack, combatStats[i].type,
                                   combatStats[i].speed, combatStats[i].defense[0], combatStats[i].defense[1],
                                   combatStats[i].defense[2]);
        });
    return Save::CreateMaterial(b, id, sName, amount, consumption, demandSlope, demandIntercept, sPerishCounters,
                                sCombatStats);
}

void dropTrail(std::string *t, unsigned int dK) {
    // Trim decimal places beyond dK from string t.
    size_t dP; // position to drop
    dP = t->find('.') + dK;
    if (dP < std::string::npos)
        t->erase(dP);
}
