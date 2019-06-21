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

#include "nation.h"

Nation::Nation(sqlite3_stmt *q, const std::vector<Good> &gs) {
    id = sqlite3_column_int(q, 0);
    names.push_back(std::string(reinterpret_cast<const char *>(sqlite3_column_text(q, 1))));
    names.push_back(std::string(reinterpret_cast<const char *>(sqlite3_column_text(q, 2))));
    adjective = std::string(reinterpret_cast<const char *>(sqlite3_column_text(q, 3)));
    foreground.r = sqlite3_column_int(q, 4);
    foreground.g = sqlite3_column_int(q, 5);
    foreground.b = sqlite3_column_int(q, 6);
    background.r = sqlite3_column_int(q, 7);
    background.g = sqlite3_column_int(q, 8);
    background.b = sqlite3_column_int(q, 9);
    highlight.r = 255 - (foreground.r + background.r) / 2;
    highlight.g = 255 - (foreground.g + background.g) / 2;
    highlight.b = 255 - (foreground.b + background.b) / 2;
    religion = std::string(reinterpret_cast<const char *>(sqlite3_column_text(q, 10)));
    goods = gs;
}

bool Nation::operator==(const Nation &other) const { return id == other.id; }

double Nation::getFrequency(int b, int m) const {
    auto it = frequencies.find(std::make_pair(b, m));
    if (it == frequencies.end())
        return 0;
    else
        return it->second;
}

std::string Nation::randomName() const {
    std::uniform_int_distribution<> dis(0, travelerNames.size() - 1);
    return travelerNames[dis(*Settings::getRng())];
}

void Nation::loadData(sqlite3 *c) {
    // Load traveler names, business frequencies map, and good consumption data for this nation.
    sqlite3_stmt *quer;
    sqlite3_prepare_v2(c, "SELECT name FROM names WHERE nation_id = ?", -1, &quer, nullptr);
    sqlite3_bind_int(quer, 1, id);
    while (sqlite3_step(quer) != SQLITE_DONE)
        travelerNames.push_back(std::string(reinterpret_cast<const char *>(sqlite3_column_text(quer, 0))));
    sqlite3_finalize(quer);
    sqlite3_prepare_v2(c,
                       "SELECT business_id, mode, frequency FROM "
                       "frequencies WHERE nation_id = ?",
                       -1, &quer, nullptr);
    sqlite3_bind_int(quer, 1, id);
    while (sqlite3_step(quer) != SQLITE_DONE)
        frequencies.emplace(std::make_pair(sqlite3_column_int(quer, 0), sqlite3_column_int(quer, 1)),
                            sqlite3_column_double(quer, 2));
    sqlite3_finalize(quer);
    // Load this nation's consumption information for each material of each good.
    sqlite3_prepare_v2(c, "SELECT * FROM consumption WHERE nation_id = ?", -1, &quer, nullptr);
    sqlite3_bind_int(quer, 1, id);
    int oGId = 0;
    std::unordered_map<int, std::array<double, 3>> consumptions;
    while (sqlite3_step(quer) != SQLITE_DONE) {
        int gId = sqlite3_column_int(quer, 1);
        if (gId != oGId) {
            // Good id changed, flush consumptions map.
            goods[oGId].assignConsumption(consumptions);
            consumptions.clear();
            oGId = gId;
        }
        int mId = sqlite3_column_int(quer, 2);
        std::array<double, 3> consumption{
            {sqlite3_column_double(quer, 3), sqlite3_column_double(quer, 4), sqlite3_column_double(quer, 5)}};
        consumptions.emplace(std::make_pair(mId, consumption));
    }
}
