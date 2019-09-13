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

Good::Good(const Save::Good *ldGd)
    : goodId(ldGd->goodId()), materialId(ldGd->materialId()), fullId(ldGd->fullId()),
      goodName(ldGd->goodName()->str()), materialName(ldGd->materialName()->str()), fullName(completeName()),
      amount(ldGd->amount()), perish(ldGd->perish()), carry(ldGd->carry()), measure(ldGd->measure()->str()),
      split(!measure.empty()), consumptionRate(ldGd->consumptionRate()), demandSlope(ldGd->demandSlope()),
      demandIntercept(ldGd->demandIntercept()), minPrice(demandIntercept / Settings::getMinPriceDivisor()),
      lastAmount(amount) {}

flatbuffers::Offset<Save::Good> Good::save(flatbuffers::FlatBufferBuilder &b) const {
    auto svGoodName = b.CreateString(goodName);
    auto svMaterialName = b.CreateString(materialName);
    auto svMeasure = b.CreateString(measure);
    auto svPerishCounters = b.CreateVectorOfStructs<Save::PerishCounter>(
        perishCounters.size(), [this](size_t i, Save::PerishCounter *pC) {
            *pC = Save::PerishCounter(perishCounters[i].time, perishCounters[i].amount);
        });
    auto svCombatStats =
        b.CreateVectorOfStructs<Save::CombatStat>(combatStats.size(), [this](size_t i, Save::CombatStat *cS) {
            *cS = Save::CombatStat(
                static_cast<Save::Part>(combatStats[i].part), static_cast<Save::Stat>(combatStats[i].stat),
                combatStats[i].attack, combatStats[i].speed,
                static_cast<Save::AttackType>(combatStats[i].type), combatStats[i].defense[AttackType::bash],
                combatStats[i].defense[AttackType::slash], combatStats[i].defense[AttackType::stab]);
        });
    return Save::CreateGood(b, goodId, materialId, fullId, svGoodName, svMaterialName, amount, perish, carry, svMeasure,
                            consumptionRate, demandSlope, demandIntercept, svPerishCounters, svCombatStats, shoots);
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
    return std::max(demandIntercept - demandSlope * (amount + qtt / 2), minPrice) * qtt;
}

double Good::price() const {
    // Get the current price of this good.
    return std::max(demandIntercept - demandSlope * amount, minPrice);
}

double Good::cost(double &qtt) const {
    // Get the cost to buy the given quantity, or available amount if less.
    qtt = std::min(amount, qtt);
    return std::max(demandIntercept - demandSlope * (amount - qtt / 2), minPrice) * qtt;
}

double Good::quantity(double cst, double &exc) const {
    /* Get quantity of this material available for given cost.
     * Second parameter holds excess quantity after amount is used up. */
    double qtt;
    double b = demandIntercept - demandSlope * amount;
    if (demandSlope != 0)
        qtt = amount - (demandIntercept - sqrt(b * b + demandSlope * cst * 2)) / demandSlope;
    else if (demandIntercept != 0)
        qtt = cst / demandIntercept;
    else
        qtt = 0;
    if (qtt < 0) return 0;
    if (amount > qtt) return qtt;
    // There's not enough good to sell in the town.
    exc = qtt - amount;
    return amount;
}

double Good::quantity(double cst) const {
    // Get quantity of this material offered for given price. Ignore material availability.
    double qtt;
    double b = demandIntercept - demandSlope * amount;
    if (demandSlope != 0)
        qtt = amount - (demandIntercept - sqrt(b * b + demandSlope * cst * 2)) / demandSlope;
    else if (demandIntercept != 0)
        qtt = cst / demandIntercept;
    else
        qtt = 0;
    if (qtt < 0) return 0;
    return qtt;
}

double Good::quota(double prc) const {
    // Get quota of this material needed to achieve given price.
    double qta;
    double b = demandIntercept - demandSlope * amount;
    if (demandSlope != 0)
        qta = amount - (demandIntercept - sqrt(b * b - demandSlope * prc * 2)) / demandSlope;
    else if (demandIntercept != 0)
        qta = prc / demandIntercept;
    else
        qta = 0;
    if (qta < 0) return 0;
    return qta;
}

void Good::setConsumption(const std::array<double, 3> &cnsptn) {
    // Assign the given three values to consumption, demand slope, and demand intercept.
    consumptionRate = cnsptn[0];
    demandSlope = cnsptn[1];
    demandIntercept = cnsptn[2];
}

void Good::scale(double ppl) {
    // Assign consumption to match given population.
    demandSlope /= ppl;
    consumptionRate *= ppl;
}

void Good::enforceMaximum() {
    // Removes materials in excess of max
    if (maximum > 0 && maximum < amount) { use(amount - maximum); }
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
    auto middle = perishCounters.insert(end(perishCounters), std::make_move_iterator(begin(gd.perishCounters)),
                                        std::make_move_iterator(end(gd.perishCounters)));
    std::inplace_merge(begin(perishCounters), middle, end(perishCounters));
}

void Good::use(double amt) {
    // Uses up the given amount of this good.
    if (amt < 0) return;
    amount = std::max(amount - amt, 0.);
    while (amt > 0 && !perishCounters.empty()) {
        // Amount and amt will count down as perish counters are used.
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
    if (std::isnan(amount)) throw std::runtime_error(fullName + " amount is " + std::to_string(amount));
}

void Good::create(double amt) {
    // Newly creates the given amount of this good.
    amount += amt;
    if (amt > 0 && perish != 0) perishCounters.push_front({0, amt});
    enforceMaximum();
    if (std::isnan(amount)) throw std::runtime_error(fullName + " amount is " + std::to_string(amount));
}

void Good::update(unsigned int elTm, double dyLn) {
    // Remove consumed goods and perished goods over elapsed time. Return amount changed.
    lastAmount = amount;
    double consumed = consumptionRate * static_cast<double>(elTm) / dyLn;
    // Ensure we don't consume more than current amount.
    consumed = std::min(amount, consumed);
    if (consumed > 0)
        // Positive consumption uses goods.
        use(consumed);
    else if (consumed < 0)
        // Negative consumption creates goods.
        create(-consumed);
    if (perishCounters.empty()) return;
    // Find the first perish counter that will expire.
    PerishCounter exPC = {int(perish * dyLn - elTm), 0};
    auto expired = std::upper_bound(perishCounters.begin(), perishCounters.end(), exPC);
    // Total expired amounts.
    double perished =
        std::accumulate(expired, perishCounters.end(), 0., [](double d, auto &pC) { return d + pC.amount; });
    // Erase expired perish counters.
    perishCounters.erase(expired, perishCounters.end());
    // Add elapsed time to remaining counters.
    for (auto &pC : perishCounters) pC.time += elTm;
    perished = std::min(amount, perished);
    amount -= perished;
}

std::unique_ptr<MenuButton> Good::button(bool aS, BoxInfo &bI, Printer &pr) const {
    bI.text = {fullName};
    bI.id = {fullId, false};
    if (image) bI.images = {{image, {2 * bI.size.border, bI.rect.h / 2 - image->h / 2, image->w, image->h}}};
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

void Good::updateButton(std::string &aT, TextBox *btn) const {
    // Finish updating button.
    std::string changeText = std::to_string(amount - lastAmount);
    dropTrail(changeText, 5);
    dropTrail(aT, split ? 3 : 0);
    btn->setText({btn->getText(0), aT, changeText});
}

void Good::updateButton(TextBox *btn) const {
    // Update amount shown on this material's button. Call when no offer or request has been made.
    std::string amountText;
    amountText = std::to_string(amount);
    updateButton(amountText, btn);
}

void Good::updateButton(double oV, unsigned int rC, TextBox *btn) const {
    // Update amount shown on this material's button. Call only when offer value and request count are non-zero.
    std::string amountText;
    if (rC)
        amountText = std::to_string(std::min(quantity(oV / rC * Settings::getTownProfit()), amount));
    else
        amountText = std::to_string(amount);
    updateButton(amountText, btn);
}

void Good::adjustDemand(double dDS) {
    // Change demand slope by parameter * demandIntercept.
    demandSlope += dDS * demandIntercept;
    demandSlope = std::max(0., demandSlope);
}

void Good::fixDemand(double m) {
    // Check if price goes too low when good is at maximum.
    if (demandIntercept - demandSlope * m < minPrice) demandSlope = (demandIntercept - minPrice) / m;
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
