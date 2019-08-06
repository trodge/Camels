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

flatbuffers::Offset<Save::Good> Good::save(flatbuffers::FlatBufferBuilder &b) const {
    auto svGoodName = b.CreateString(goodName);
    auto svMaterialName = b.CreateString(materialName);
    auto svMeasure = b.CreateString(measure);
    auto svPerishCounters =
        b.CreateVectorOfStructs<Save::PerishCounter>(perishCounters.size(), [this](size_t i, Save::PerishCounter *pC) {
            *pC = Save::PerishCounter(perishCounters[i].time, perishCounters[i].amount);
        });
    auto svCombatStats = b.CreateVectorOfStructs<Save::CombatStat>(combatStats.size(), [this](size_t i, Save::CombatStat *cS) {
        *cS = Save::CombatStat(combatStats[i].statId, combatStats[i].partId, combatStats[i].attack, combatStats[i].type,
                               combatStats[i].speed, combatStats[i].defense[0], combatStats[i].defense[1],
                               combatStats[i].defense[2]);
    });
    return Save::CreateGood(b, goodId, materialId, index, svGoodName, svMaterialName, amount, perish, carry, svMeasure,
                            consumptionRate, demandSlope, demandIntercept, svPerishCounters, svCombatStats, ammoId);
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
    if (split || amount == 1. || goodName == "sheep")
        bsnTx = goodName + ": " + bsnTx;
    else
        bsnTx = goodName + "s: " + bsnTx;
    return bsnTx;
}

void Good::setFullName() { fullName = goodName == materialName ? goodName : goodName + " " + materialName; }

std::string Good::logEntry() const {
    std::string logText = std::to_string(amount);
    dropTrail(logText, split ? 3 : 0);
    if (split) {
        logText += " " + measure;
        if (amount != 1) logText += "s";
        logText += " of";
    }
    logText += " " + fullName;
    if (!split && amount != 1)
        // Unsplitable goods don't use measure words.
        logText += "s";
    return logText;
}

double Good::price(double qtt) const {
    // Get the price offered when selling the given quantity.
    return std::max((demandIntercept - demandSlope * (amount + qtt / 2)) * qtt, minPrice * qtt);
}

double Good::price() const {
    // Get the current price of this good.
    return std::max(demandIntercept - demandSlope * amount, minPrice);
}

double Good::cost(double q) const {
    // Get the cost to buy the given quantity
    return std::max((demandIntercept - demandSlope * (amount - q / 2)) * q, minPrice * q);
}

double Good::quantity(double p, double &e) const {
    /* Get quantity of this material available at given price.
     * Second parameter holds excess quantity after amount is used up. */
    double q;
    double b = demandIntercept - demandSlope * amount;
    if (demandSlope != 0)
        q = amount - (demandIntercept - sqrt(b * b + demandSlope * p * 2)) / demandSlope;
    else if (demandIntercept != 0)
        q = p / demandIntercept;
    else
        q = 0;
    if (q < 0) return 0;
    if (amount > q) return q;
    // There's not enough good to sell in the town.
    e = q - amount;
    return amount;
}

double Good::quantity(double p) const {
    // Get quantity of this material corresponding to price. Ignore material availability.
    double q;
    double b = demandIntercept - demandSlope * amount;
    if (demandSlope != 0)
        q = amount - (demandIntercept - sqrt(b * b + demandSlope * p * 2)) / demandSlope;
    else if (demandIntercept != 0)
        q = p / demandIntercept;
    else
        q = 0;
    if (q < 0) return 0;
    return q;
}

double Good::quantum(double c) const {
    // Get quantum of this material needed to match given cost.
    double q;
    double b = demandIntercept - demandSlope * amount;
    if (demandSlope != 0)
        q = amount - (demandIntercept - sqrt(b * b - demandSlope * c * 2)) / demandSlope;
    else if (demandIntercept != 0)
        q = c / demandIntercept;
    else
        q = 0;
    if (q < 0) return 0;
    return q;
}

void Good::setConsumption(const std::array<double, 3> &cnsptn) {
    // Assign the given three values to consumption, demand slope, and demand intercept.
    consumptionRate = cnsptn[0];
    demandSlope = cnsptn[1];
    demandIntercept = cnsptn[2];
}

void Good::scaleConsumption(unsigned long ppl) {
    // Assign consumption to match given population.
    demandSlope /= static_cast<double>(ppl);
    consumptionRate *= static_cast<double>(ppl);
}

void Good::enforceMaximum() {
    // Removes materials in excess of max
    if (maximum > 0 && amount > maximum) { amount = maximum; }
}

void Good::take(Good &gd) {
    // Takes the given good from this good. Stores perish counters for transfer in parameter.
    amount -= gd.amount;
    if (amount < 0) {
        gd.amount += amount;
        amount = 0;
    }
    double movedAmount = gd.getAmount(); // amount moved not accounted for by transfered perish counters
    while (movedAmount > 0 && !perishCounters.empty()) {
        // Moved amount is goingd down.
        PerishCounter pC = perishCounters.back(); // copy of most recently added perish counter
        perishCounters.pop_back();
        if (pC.amount > movedAmount) {
            // Perish counter is enough to make change and stay around.
            pC.amount -= movedAmount;
            // Put perish counter back less amount moved.
            perishCounters.push_back(pC);
            // Taken perish counter has just the amount taken.
            pC.amount = movedAmount;
            gd.perishCounters.push_front(pC);
            movedAmount = 0;
        } else {
            // Perish counter is used up to make change.
            gd.perishCounters.push_front(pC);
            movedAmount -= pC.amount;
        }
    }
}

void Good::put(Good &gd) {
    // Puts the given good in this good. Transfers perish counters from parameter.
    amount += gd.amount;
    auto middle = perishCounters.insert(perishCounters.end(), gd.perishCounters.begin(), gd.perishCounters.end());
    std::inplace_merge(perishCounters.begin(), middle, perishCounters.end());
}

void Good::use(double amt) {
    // Uses up the given amount of this good.
    if (amt > 0) {
        if (amount >= amt)
            amount -= amt;
        else
            amount = 0;
    }
    while (amt > 0 && !perishCounters.empty()) {
        // Amount is going down.
        PerishCounter pC = perishCounters.back();
        perishCounters.pop_back();
        if (pC.amount > amt) {
            // Perish counter is enough to make change and stay around.
            pC.amount -= amt;
            perishCounters.push_back(pC);
            amt = 0;
        } else {
            // Perish counter is used up to make change.
            amt -= pC.amount;
        }
    }
}

void Good::create(double amt) {
    // Newly creates the given amount of this good.
    amount += amt;
    if (amt > 0 && perish != 0) perishCounters.push_front({0, amt});
    enforceMaximum();
}

void Good::update(unsigned int elTm) {
    // Remove consumed goods and perished goods over elapsed time.
    lastAmount = amount;
    double consumption = consumptionRate * elTm / kDaysPerYear / Settings::getDayLength();
    if (consumption > amount) consumption = amount;
    if (consumption > 0)
        use(consumption);
    else if (consumption < 0)
        create(-consumption);
    if (perish == 0) return;
    // Find the first perish counter that will expire.
    PerishCounter exPC = {int(perish * Settings::getDayLength() * kDaysPerYear - elTm), 0};
    auto expired = std::upper_bound(perishCounters.begin(), perishCounters.end(), exPC);
    // Remove expired amounts from amount and total them.
    double amt =
        std::accumulate(expired, perishCounters.end(), 0, [](double d, const PerishCounter &pC) { return d + pC.amount; });
    // Erase expired perish counters.
    perishCounters.erase(expired, perishCounters.end());
    // Add elapsed time to remaining counters.
    for (auto &pC : perishCounters) pC.time += elTm;
    // If too much perished, reduce total by overage.
    if (amount > amt) {
        amount -= amt;
    } else {
        amt = amount;
        amount = 0;
    }
    if (amount < 0) amount = 0;
    // Keep demand slopes from being too low or too high
    if (demandIntercept - demandSlope * maximum < minPrice)
        // Price goes too low when good is at maximum.
        demandSlope = (demandIntercept - minPrice) / maximum;
}

std::unique_ptr<MenuButton> Good::button(bool aS, BoxInfo bI, Printer &pr) const {
    bI.text = {fullName};
    bI.id = index;
    if (image) bI.images = {{image, {2 * bI.border, bI.rect.h / 2 - image->h / 2, image->w, image->h}}};
    if (aS) {
        // Button will have amount shown.
        std::string amountText = std::to_string(amount);
        dropTrail(amountText, split ? 3 : 0);
        bI.text.push_back(std::move(amountText));
        return std::make_unique<MenuButton>(bI, pr);
    }
    // Button does not show amount.
    return std::make_unique<MenuButton>(bI, pr);
}

void Good::updateButton(std::string &aT, bool gS, TextBox *btn) const {
    // Finish updating button.
    std::string changeText = std::to_string(amount - lastAmount);
    dropTrail(changeText, 5);
    dropTrail(aT, gS ? 3 : 0);
    btn->setText({btn->getText(0u), aT, changeText});
}

void Good::updateButton(bool gS, TextBox *btn) const {
    // Update amount shown on this material's button. Call when no offer or request has been made.
    std::string amountText;
    amountText = std::to_string(amount);
    updateButton(amountText, gS, btn);
}

void Good::updateButton(bool gS, double oV, unsigned int rC, TextBox *btn) const {
    // Update amount shown on this material's button. Call only when offer value and request count are non-zero.
    std::string amountText;
    if (rC)
        amountText = std::to_string(std::min(quantity(oV / rC * Settings::getTownProfit()), amount));
    else
        amountText = std::to_string(amount);
    updateButton(amountText, gS, btn);
}

void Good::adjustDemandSlope(double dDS) {
    // Change demand slope by parameter * demandIntercept.
    demandSlope += dDS * demandIntercept;
    demandSlope = std::max(0., demandSlope);
}

void Good::saveDemand(unsigned long ppl, std::string &u) const {
    u.append(" WHEN good_id = " + std::to_string(goodId) + " AND material_id = " + std::to_string(materialId) +
             " THEN " + std::to_string(demandSlope * static_cast<double>(ppl)));
}

void dropTrail(std::string &tx, unsigned int dK) {
    // Trim decimal places beyond dK from string tx.
    size_t dP; // position to drop
    dP = tx.find('.') + dK;
    if (dP < std::string::npos) tx.erase(dP);
}