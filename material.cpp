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

#include "material.hpp"

Material::Material(const Save::Material *m)
    : id(static_cast<unsigned int>(m->id())), name(m->name()->str()), amount(m->amount()), perish(m->perish()),
      carry(m->carry()), consumption(m->consumption()), demandSlope(m->demandSlope()),
      demandIntercept(m->demandIntercept()), minPrice(demandIntercept / 63) {
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

flatbuffers::Offset<Save::Material> Material::save(flatbuffers::FlatBufferBuilder &b) const {
    auto sName = b.CreateString(name);
    auto sPerishCounters =
        b.CreateVectorOfStructs<Save::PerishCounter>(perishCounters.size(), [this](size_t i, Save::PerishCounter *pC) {
            *pC = Save::PerishCounter(perishCounters[i].time, perishCounters[i].amount);
        });
    auto sCombatStats = b.CreateVectorOfStructs<Save::CombatStat>(combatStats.size(), [this](size_t i, Save::CombatStat *cS) {
        *cS = Save::CombatStat(combatStats[i].statId, combatStats[i].partId, combatStats[i].attack, combatStats[i].type,
                               combatStats[i].speed, combatStats[i].defense[0], combatStats[i].defense[1],
                               combatStats[i].defense[2]);
    });
    return Save::CreateMaterial(b, id, sName, amount, perish, carry, consumption, demandSlope, demandIntercept,
                                sPerishCounters, sCombatStats);
}

double Material::price(double q) const {
    // Get the price offered when selling the given quantity
    double p = (demandIntercept - demandSlope * (amount + q / 2)) * q;
    if (p < minPrice * q) p = minPrice * q;
    return p;
}

double Material::price() const {
    // Get the current price of this material
    double p = demandIntercept - demandSlope * amount;
    if (p < minPrice) p = minPrice;
    return p;
}

double Material::cost(double q) const {
    // Get the cost to buy the given quantity
    double c = (demandIntercept - demandSlope * (amount - q / 2)) * q;
    if (c < minPrice * q) c = minPrice * q;
    return c;
}

double Material::quantity(double p, double &e) const {
    /* Get quantity of this material available at given price.
     * Second parameter holds excess quantity after amount is used up. */
    double q;
    double b = demandIntercept - demandSlope * amount;
    if (demandSlope != 0.)
        q = amount - (demandIntercept - sqrt(b * b + demandSlope * p * 2)) /
                         demandSlope;
    else if (demandIntercept != 0.)
        q = p / demandIntercept;
    else
        q = 0;
    if (q < 0) return 0;
    if (amount > q) return q;
    // There's not enough good to sell in the town.
    e = q - amount;
    return amount;
}

double Material::quantity(double p) const {
    // Get quantity of this material corresponding to price. Ignore material availability.
    double q;
    double b = demandIntercept - demandSlope * amount;
    if (demandSlope != 0.)
        q = amount - (demandIntercept - sqrt(b * b + demandSlope * p * 2)) /
                         demandSlope;
    else if (demandIntercept != 0.)
        q = p / demandIntercept;
    else
        q = 0;
    if (q < 0) return 0;
    return q;
}

double Material::quantum(double c) const {
    // Get quantum of this material needed to match given cost.
    double q;
    double b = demandIntercept - demandSlope * amount;
    if (demandSlope != 0.)
        q = amount - (demandIntercept - sqrt(b * b - demandSlope * c * 2)) /
                         demandSlope;
    else if (demandIntercept != 0.)
        q = c / demandIntercept;
    else
        q = 0;
    if (q < 0) return 0;
    return q;
}

void Material::setConsumption(const std::array<double, 3> &c) {
    // Assign the given three values to consumption, demand slope, and demand intercept.
    consumption = c[0];
    demandSlope = c[1];
    demandIntercept = c[2];
}

void Material::scaleConsumption(unsigned long p) {
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
    while (movedAmount > 0 && !perishCounters.empty()) {
        // Amount is going down.
        PerishCounter pC = perishCounters.back();
        perishCounters.pop_back();
        if (pC.amount > movedAmount) {
            // Perish counter is enough to make change and stay around.
            pC.amount -= movedAmount;
            perishCounters.push_back(pC);
            // Taken pc has just the amount taken.
            pC.amount = movedAmount;
            m.perishCounters.push_front(pC);
            movedAmount = 0;
        } else {
            // Perish counter is used up to make change.
            m.perishCounters.push_front(pC);
            movedAmount -= pC.amount;
        }
    }
}

void Material::use(double a) {
    // Use the given amount and discard the perish counters.
    if (a > 0. && amount >= a)
        amount -= a;
    else
        a = 0.;
    while (a > 0. && !perishCounters.empty()) {
        // Amount is going down.
        PerishCounter pC = perishCounters.back();
        perishCounters.pop_back();
        if (pC.amount > a) {
            // Perish counter is enough to make change and stay around.
            pC.amount -= a;
            perishCounters.push_back(pC);
            a = 0.;
        } else {
            // Perish counter is used up to make change.
            a -= pC.amount;
        }
    }
}

void Material::put(Material &m) {
    amount += m.amount;
    auto middle = perishCounters.insert(end(perishCounters), begin(m.perishCounters), end(m.perishCounters));
    std::inplace_merge(begin(perishCounters), middle, end(perishCounters));
}

void Material::create(double a) {
    // Newly create the given amount of this material.
    amount += a;
    if (a > 0) perishCounters.push_front({0, a});
}

double Material::consume(unsigned int e) {
    // Consume the material for e milliseconds.
    lastAmount = amount;
    double c = consumption * e / kDaysPerYear / Settings::getDayLength();
    if (c > amount) c = amount;
    if (c > 0.)
        use(c);
    else if (c < 0.)
        create(-c);
    // Find the first perish counter that will expire.
    PerishCounter ePC = {int(perish * Settings::getDayLength() * kDaysPerYear - e), 0};
    auto expired = std::upper_bound(begin(perishCounters), end(perishCounters), ePC);
    // Remove expired amounts from amount and total them.
    double p =
        std::accumulate(expired, end(perishCounters), 0., [](double d, const PerishCounter &pC) { return d + pC.amount; });
    // Erase expired perish counters.
    perishCounters.erase(expired, end(perishCounters));
    // Add elapsed time to remaining counters.
    for (auto &pC : perishCounters) pC.time += e;
    // If too much perished, reduce total by overage.
    if (amount > p) {
        amount -= p;
    } else {
        p = amount;
        amount = 0;
    }
    // Return total amount perished and consumed.
    return c + p;
}

void Material::updateButton(std::string &aT, bool gS, TextBox *b) const {
    // Finish updating button.
    std::string changeText = std::to_string(amount - lastAmount);
    dropTrail(changeText, 4);
    dropTrail(aT, gS ? 3 : 0);
    b->setText({b->getText()[0], aT, changeText});
}

void Material::updateButton(bool gS, TextBox *b) const {
    // Update amount shown on this material's button. Call when no offer or request has been made.
    std::string amountText;
    amountText = std::to_string(amount);
    updateButton(amountText, gS, b);
}

void Material::updateButton(bool gS, double oV, unsigned int rC, TextBox *b) const {
    // Update amount shown on this material's button. Call only when offer value and request count are non-zero.
    std::string amountText;
    if (rC)
        amountText = std::to_string(std::min(quantity(oV / rC * Settings::getTownProfit()), amount));
    else
        amountText = std::to_string(amount);
    updateButton(amountText, gS, b);
}

void Material::adjustDemand(double d) {
    demandSlope += d * demandIntercept;
    if (demandSlope < 0) demandSlope = 0;
}

void Material::fixDemand(double m) {
    // Check if price goes too low when good is at maximum.
    if (demandIntercept - demandSlope * m < minPrice) demandSlope = (demandIntercept - minPrice) / m;
}

void Material::saveDemand(unsigned long p, std::string &u) const {
    u.append(" AND material_id = ");
    u.append(std::to_string(id));
    u.append(" THEN ");
    u.append(std::to_string(demandSlope * static_cast<double>(p)));
}

void dropTrail(std::string &tx, unsigned int dK) {
    // Trim decimal places beyond dK from string t.
    size_t dP; // position to drop
    dP = tx.find('.') + dK;
    if (dP < tx.size()) tx.erase(dP);
}
