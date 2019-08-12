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

#include "business.hpp"

Business::Business(unsigned int i, unsigned int m, const std::string &nm, bool cS, bool rC, bool kM,
                   const std::array<double, 3> &fFs)
    : id(i), mode(m), name(nm), area(1), canSwitch(cS), requireCoast(rC), keepMaterial(kM), frequency(1),
      frequencyFactors(fFs) {}

Business::Business(const Save::Business *b)
    : id(b->id()), mode(b->mode()), name(b->name()->str()), area(b->area()), canSwitch(b->canSwitch()),
      requireCoast(b->requireCoast()), keepMaterial(b->keepMaterial()), frequency(b->frequency()),
      reclaimFactor(b->reclaimFactor()) {
    auto lRequirements = b->requirements();
    for (auto lII = lRequirements->begin(); lII != lRequirements->end(); ++lII) requirements.push_back(Good(*lII));
    auto lReclaimables = b->reclaimables();
    for (auto lOI = lReclaimables->begin(); lOI != lReclaimables->end(); ++lOI) reclaimables.push_back(Good(*lOI));
    auto lInputs = b->inputs();
    for (auto lII = lInputs->begin(); lII != lInputs->end(); ++lII) inputs.push_back(Good(*lII));
    auto lOutputs = b->outputs();
    for (auto lOI = lOutputs->begin(); lOI != lOutputs->end(); ++lOI) outputs.push_back(Good(*lOI));
}

flatbuffers::Offset<Save::Business> Business::save(flatbuffers::FlatBufferBuilder &b) const {
    auto sName = b.CreateString(name);
    auto sRequirements = b.CreateVector<flatbuffers::Offset<Save::Good>>(
        requirements.size(), [this, &b](size_t i) { return requirements[i].save(b); });
    auto sReclaimables = b.CreateVector<flatbuffers::Offset<Save::Good>>(
        reclaimables.size(), [this, &b](size_t i) { return reclaimables[i].save(b); });
    auto sInputs =
        b.CreateVector<flatbuffers::Offset<Save::Good>>(inputs.size(), [this, &b](size_t i) { return inputs[i].save(b); });
    auto sOutputs = b.CreateVector<flatbuffers::Offset<Save::Good>>(
        outputs.size(), [this, &b](size_t i) { return outputs[i].save(b); });
    return Save::CreateBusiness(b, id, mode, sName, area, canSwitch, requireCoast, keepMaterial, sRequirements,
                                sReclaimables, sInputs, sOutputs, frequency);
}

void Business::setArea(double a) {
    if (area > 0) {
        for (auto &ip : inputs) ip.setAmount(ip.getAmount() * a / area);
        for (auto &op : outputs) op.setAmount(op.getAmount() * a / area);
        area = a;
    }
}

void Business::scale(unsigned long ppl, unsigned int tT) {
    // Set area according to given population and town type.
    setArea(static_cast<double>(ppl) * frequency * frequencyFactors[tT - 1]);
}

void Business::takeRequirements(Property &inv, double a) {
    // Take the requirments to add given area to business from parameter and store them in reclaimables.
    for (auto &rq : requirements) {
        // Take goods from property matching requirement id.
        auto tGs = inv.take(rq.getGoodId(), rq.getAmount() * a);
        for (auto &tG : tGs) {
            // Reduce transfered amount to amount determined by reclaim factor.
            tG.setAmount(tG.getAmount() * reclaimFactor);
            // Put into reclaimables.
            auto rcbIt = std::lower_bound(begin(reclaimables), end(reclaimables), tG);
            if (rcbIt == end(reclaimables) || *rcbIt != tG)
                // Good not yet in reclaimables, insert it.
                reclaimables.insert(rcbIt, tG);
            else
                rcbIt->put(tG);
        }
    }
}

void Business::reclaim(Property &inv, double a) {
    // Return reclaimables for the given area to parameter.
    for (auto &rcbl : reclaimables) {
        // Construct a transfer good matching one in reclaimables.
        Good tG(rcbl);
        // Set the transfer amount proportional to area being reclaimed.
        tG.setAmount(rcbl.getAmount() * a / area);
        // Take from reclaimables.
        rcbl.take(tG);
        // Put into property.
        inv.put(tG);
    }
}

void Business::addConflicts(std::unordered_map<unsigned int, std::pair<bool, unsigned int>> &cfts, const Property &inv) {
    // Counts number of businesses using each good and determines if good will run out this cycle.
    bool space = false;
    for (auto &op : outputs) {
        auto gId = op.getGoodId();
        auto amt = inv.amount(gId);
        auto max = inv.maximum(gId);
        if (amt < max || max == 0) {
            space = true;
            break;
        }
    }
    if (!space) {
        // No space exists for outputs.
        factor = 0;
        return;
    }
    auto maxFactor = factor;
    for (auto &ip : inputs) {
        auto gId = ip.getGoodId();
        auto inputFactor = inv.amount(gId) / ip.getAmount(); // max factor possible given this input
        if (ip == outputs.back() && inputFactor < 1)
            // For livestock, max factor is multiplicative when not enough breeding stock are available.
            maxFactor *= inputFactor;
        else if (factor > inputFactor) {
            // Not livestock and this business will use up input.
            maxFactor = std::min(inputFactor, maxFactor);
            cfts[gId].first = true;
        }
        ++cfts[gId].second;
    }
    factor = maxFactor;
    if (factor < 0)
        std::cout << factor << " factor for " << name << " area " << area << " frequency " << frequency << std::endl;
}

void Business::handleConflicts(std::unordered_map<unsigned int, std::pair<bool, unsigned int>> &cfts) {
    unsigned int grCft = 0;
    for (auto &ip : inputs) {
        auto &cft = cfts[ip.getGoodId()];
        if (cft.first) grCft = std::max(cft.second, grCft);
    }
    if (grCft) factor /= static_cast<double>(grCft);
}

void Business::run(Property &inv) {
    // Run this business on the given goods.
    if (factor > 0) {
        auto lastInputId = inputs.back().getGoodId(); // inputs which determine material
        for (auto &op : outputs) {
            auto outputId = op.getGoodId();
            if (keepMaterial && outputId != lastInputId)
                inv.create(outputId, lastInputId, op.getAmount() * factor);
            else
                // Materials are not kept or last input and output are the same, ignore materials.
                inv.create(outputId, op.getAmount() * factor);
        }
        for (auto &ip : inputs) inv.use(ip.getGoodId(), ip.getAmount() * factor);
    }
}

std::unique_ptr<MenuButton> Business::button(bool aS, BoxInfo bI, Printer &pr) const {
    // Create a button for this business using the given box info.
    bI.text = {name};
    std::string unitText; // Units of input and output post-fix.
    if (aS) {
        std::string areaText = std::to_string(area);
        dropTrail(areaText, 2);
        bI.text.push_back("Area: " + areaText + " uncia");
        bI.text.push_back("Factor: " + std::to_string(factor));
        unitText = " per annum";
    } else
        unitText = " per uncia anum";
    bI.text.push_back("Requirements per uncia");
    for (auto &rq : requirements) bI.text.push_back(rq.businessText());
    bI.text.push_back("Inputs" + unitText);
    for (auto &ip : inputs) bI.text.push_back(ip.businessText());
    bI.text.push_back("Outputs" + unitText);
    for (auto &op : outputs) bI.text.push_back(op.businessText());
    return std::make_unique<MenuButton>(bI, pr);
}

void Business::saveFrequency(unsigned long p, std::string &u) const {
    if (frequency > 0) {
        u.append(" WHEN business_id = ");
        u.append(std::to_string(id));
        u.append(" AND mode = ");
        u.append(std::to_string(mode));
        u.append(" THEN ");
        u.append(std::to_string(area / static_cast<double>(p)));
    }
}
