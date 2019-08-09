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

#include "nation.hpp"

Nation::Nation(unsigned int i, const std::vector<std::string> &nms, const std::string &adj, const SDL_Color &fgr,
               const SDL_Color &bgr, const std::string &rlg, const std::vector<Good> &gds, const std::vector<Business> &bsns)
    : id(i), names(nms), adjective(adj), foreground(fgr), background(bgr),
      dot({static_cast<Uint8>((fgr.r + bgr.r) / 2), static_cast<Uint8>((fgr.g + bgr.g) / 2),
           static_cast<Uint8>((fgr.b + bgr.b) / 2), 255}),
      highlight({static_cast<Uint8>(255 - dot.r), static_cast<Uint8>(255 - dot.g), static_cast<Uint8>(255 - dot.b), 255}),
      religion(rlg), property(i, gds, bsns) {}

bool Nation::operator==(const Nation &other) const { return id == other.id; }

std::string Nation::randomName() const {
    std::uniform_int_distribution<size_t> dis(0, travelerNames.size() - 1);
    return travelerNames[dis(Settings::rng)];
}

void Nation::setConsumption(const std::vector<std::array<double, 3>> &gdsCnsptn) {
    // Takes a vector of consumption rates, demand slopes, and demand intercepts for this nation's goods' materials
    property.setConsumption(gdsCnsptn);
}

void Nation::setFrequencies(const std::vector<double> &frqcs) { property.setFrequencies(frqcs); }
