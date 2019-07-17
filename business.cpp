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

Business::Business(unsigned int i, unsigned int m, const std::string &nm, bool cS, bool rC, bool kM)
    : id(i), mode(m), name(nm), area(1.), canSwitch(cS), requireCoast(rC), keepMaterial(kM), frequency(1.) {}

Business::Business(const Save::Business *b)
    : id(b->id()), mode(b->mode()), name(b->name()->str()), area(b->area()), canSwitch(b->canSwitch()),
      requireCoast(b->requireCoast()), keepMaterial(b->keepMaterial()), frequency(b->frequency()),
      reclaimFactor(b->reclaimFactor()) {
    auto lRequirements = b->requirements();
    for (auto lII = lRequirements->begin(); lII != lRequirements->end(); ++lII)
        requirements.push_back(Good(*lII));
    auto lReclaimables = b->reclaimables();
    for (auto lOI = lReclaimables->begin(); lOI != lReclaimables->end(); ++lOI)
        reclaimables.push_back(Good(*lOI));
    auto lInputs = b->inputs();
    for (auto lII = lInputs->begin(); lII != lInputs->end(); ++lII)
        inputs.push_back(Good(*lII));
    auto lOutputs = b->outputs();
    for (auto lOI = lOutputs->begin(); lOI != lOutputs->end(); ++lOI)
        outputs.push_back(Good(*lOI));
}

bool Business::operator==(const Business &other) const { return (id == other.id and mode == other.mode); }

bool Business::operator<(const Business &other) const { return (id < other.id or (id == other.id and mode < other.mode)); }

void Business::setArea(double a) {
    if (a > 0. and area > 0.) {
        for (auto &ip : inputs)
            ip.setAmount(ip.getAmount() * a / area);
        for (auto &op : outputs)
            op.setAmount(op.getAmount() * a / area);
        area = a;
    }
}

void Business::takeRequirements(std::vector<Good> &gds, double a) {
    // Take the requirments to add given area to business from parameter and store them in reclaimables.
    for (auto &rq : requirements) {
        auto &g = gds[rq.getId()];
        // Construct a transfer good with materials proportional to those in g.
        Good tG(g);
        tG.setAmount(rq.getAmount() * a);
        // Take from g.
        g.take(tG);
        // Reduce transfered amount to amount determined by reclaim factor.
        tG.setAmount(tG.getAmount() * reclaimFactor);
        // Put into reclaimables.
        auto rcbIt = std::lower_bound(reclaimables.begin(), reclaimables.end(), rq);
        if (*rcbIt == rq)
            rcbIt->put(tG);
        else
            reclaimables.insert(rcbIt, tG);
    }
}

void Business::reclaim(std::vector<Good> &gds, double a) {
    // Return reclaimables for the given area to parameter.
    for (auto &rq : requirements) {
        auto rcbIt = std::lower_bound(reclaimables.begin(), reclaimables.end(), rq);
        // Construct a transfer good with materials proportional to those in reclaimables.
        Good tG(*rcbIt);
        tG.setAmount(rq.getAmount() * a);
        // Take from reclaimables.
        rcbIt->take(tG);
        // Put into gds.
        gds[rq.getId()].put(tG);
    }
}

void Business::addConflicts(std::vector<int> &cs, std::vector<Good> &gds) {
    bool space = false;
    for (auto &op : outputs) {
        unsigned int oId = op.getId();
        if (gds[oId].getAmount() < gds[oId].getMax())
            space = true;
    }
    if (not space)
        factor = 0;
    for (auto &ip : inputs) {
        unsigned int iId = ip.getId();
        double mF = gds[iId].getAmount() / ip.getAmount();
        if (ip == outputs.back() and mF < 1)
            // For livestock, max factor is multiplicative when not enough breeding stock are available.
            factor *= mF;
        else if (factor > mF) {
            factor = mF;
            ++cs[iId];
        }
    }
    if (factor < 0)
        std::cout << "Negative factor for " << name << std::endl;
}

void Business::handleConflicts(std::vector<int> &cs) {
    int gc = 0;
    for (auto &ip : inputs) {
        int c = cs[static_cast<size_t>(ip.getId())];
        if (c > gc)
            gc = c;
    }
    if (gc)
        factor /= gc;
}

void Business::run(std::vector<Good> &gds) {
    // Run this business on the given goods.
    if (factor > 0) {
        auto &lastInput = gds[inputs.back().getId()];
        for (auto &op : outputs) {
            if (keepMaterial and op != lastInput) {
                // If last input and output are the same, ignore materials.
                std::unordered_map<unsigned int, double> materialAmounts;
                double lIA = lastInput.getAmount();
                for (auto &m : lastInput.getMaterials()) {
                    if (lIA > 0.)
                        materialAmounts.emplace(m.getId(), op.getAmount() * factor * m.getAmount() / lIA);
                }
                gds[op.getId()].create(materialAmounts);
            } else {
                gds[op.getId()].create(op.getAmount() * factor);
            }
        }
        for (auto &ip : inputs)
            gds[ip.getId()].use(ip.getAmount() * factor);
    }
}

std::unique_ptr<MenuButton> Business::button(bool aS, const SDL_Rect &rt, const SDL_Color &fgr, const SDL_Color &bgr, int b,
                                             int r, int fS, Printer &pr, const std::function<void()> &fn) {
    std::vector<std::string> tx = {name};
    std::string unitText; // Units of input and output post-fix.
    if (aS) {
        std::string areaText = std::to_string(area);
        dropTrail(areaText, 2u);
        tx.push_back("Area: " + areaText + " uncia");
        unitText = " per annum";
    } else
        unitText = " per uncia anum";
    tx.push_back("Requirements");
    for (auto &rq : requirements)
        tx.push_back(rq.businessText());
    tx.push_back("Inputs");
    for (auto &ip : inputs)
        tx.push_back(ip.businessText() + unitText);
    tx.push_back("Outputs");
    for (auto &op : outputs)
        tx.push_back(op.businessText() + unitText);
    return std::make_unique<MenuButton>(rt, tx, fgr, bgr, b, r, fS, pr, fn);
}

void Business::saveFrequency(unsigned long p, std::string &u) const {
    if (frequency > 0.) {
        u.append(" WHEN business_id = ");
        u.append(std::to_string(id));
        u.append(" AND mode = ");
        u.append(std::to_string(mode));
        u.append(" THEN ");
        u.append(std::to_string(area / static_cast<double>(p) / frequency));
    }
}

flatbuffers::Offset<Save::Business> Business::save(flatbuffers::FlatBufferBuilder &b) const {
    auto sName = b.CreateString(name);
    auto sRequirements = b.CreateVector<flatbuffers::Offset<Save::Good>>(
        requirements.size(), [this, &b](size_t i) { return requirements[i].save(b); });
    auto sReclaimables = b.CreateVector<flatbuffers::Offset<Save::Good>>(
        reclaimables.size(), [this, &b](size_t i) { return reclaimables[i].save(b); });
    auto sInputs =
        b.CreateVector<flatbuffers::Offset<Save::Good>>(inputs.size(), [this, &b](size_t i) { return inputs[i].save(b); });
    auto sOutputs =
        b.CreateVector<flatbuffers::Offset<Save::Good>>(outputs.size(), [this, &b](size_t i) { return outputs[i].save(b); });
    return Save::CreateBusiness(b, id, mode, sName, area, canSwitch, requireCoast, keepMaterial, sRequirements,
                                sReclaimables, sInputs, sOutputs, frequency);
}
