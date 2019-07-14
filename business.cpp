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

Business::Business(unsigned int i, unsigned int m, sqlite3 *c) : id(i), mode(m), area(1.) {
    // Load a new business with the given id and mode from the database
    sqlite3_stmt *quer;
    sqlite3_prepare_v2(c,
                       "SELECT name, can_switch, require_coast, keep_material FROM "
                       "businesses WHERE business_id = ?",
                       -1, &quer, nullptr);
    sqlite3_bind_int(quer, 1, static_cast<int>(i));
    sqlite3_step(quer);
    name = std::string(reinterpret_cast<const char *>(sqlite3_column_text(quer, 0)));
    canSwitch = bool(sqlite3_column_int(quer, 1));
    requireCoast = bool(sqlite3_column_int(quer, 2));
    keepMaterial = bool(sqlite3_column_int(quer, 3));
    sqlite3_finalize(quer);
    sqlite3_prepare_v2(c, "SELECT * FROM requirements WHERE business_id = ?", -1, &quer, nullptr);
    sqlite3_bind_int(quer, 1, static_cast<int>(i));
    while (sqlite3_step(quer) != SQLITE_DONE)
        requirements.push_back(Good(static_cast<unsigned int>(sqlite3_column_int(quer, 2)), sqlite3_column_double(quer, 3)));
    sqlite3_finalize(quer);
    sqlite3_prepare_v2(c, "SELECT * FROM inputs WHERE business_id = ? AND mode = ?", -1, &quer, nullptr);
    sqlite3_bind_int(quer, 1, static_cast<int>(i));
    sqlite3_bind_int(quer, 2, static_cast<int>(m));
    while (sqlite3_step(quer) != SQLITE_DONE)
        inputs.push_back(Good(static_cast<unsigned int>(sqlite3_column_int(quer, 2)), sqlite3_column_double(quer, 3)));
    sqlite3_finalize(quer);
    sqlite3_prepare_v2(c, "SELECT * FROM outputs WHERE business_id = ? AND mode = ?", -1, &quer, nullptr);
    sqlite3_bind_int(quer, 1, static_cast<int>(i));
    sqlite3_bind_int(quer, 2, static_cast<int>(m));
    while (sqlite3_step(quer) != SQLITE_DONE)
        outputs.push_back(Good(static_cast<unsigned int>(sqlite3_column_int(quer, 2)), sqlite3_column_double(quer, 3)));
    sqlite3_finalize(quer);
}

Business::Business(const Save::Business *b)
    : id(b->id()), mode(b->mode()), name(b->name()->str()), area(b->area()), canSwitch(b->canSwitch()),
      keepMaterial(b->keepMaterial()) {
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

void Business::saveFrequency(unsigned long p, std::string &u) const {
    if (frequencyFactor > 0.) {
        u.append(" WHEN business_id = ");
        u.append(std::to_string(id));
        u.append(" AND mode = ");
        u.append(std::to_string(mode));
        u.append(" THEN ");
        u.append(std::to_string(area / static_cast<double>(p) / frequencyFactor));
    }
}

flatbuffers::Offset<Save::Business> Business::save(flatbuffers::FlatBufferBuilder &b) const {
    auto sName = b.CreateString(name);
    auto sInputs =
        b.CreateVector<flatbuffers::Offset<Save::Good>>(inputs.size(), [this, &b](size_t i) { return inputs[i].save(b); });
    auto sOutputs =
        b.CreateVector<flatbuffers::Offset<Save::Good>>(outputs.size(), [this, &b](size_t i) { return outputs[i].save(b); });
    return Save::CreateBusiness(b, id, mode, sName, area, canSwitch, keepMaterial, sInputs, sOutputs, frequencyFactor);
}
