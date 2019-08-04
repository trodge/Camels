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

#ifndef GOOD_H
#define GOOD_H

#include <functional>
#include <unordered_map>
#include <vector>

#include "material.hpp"
#include "pager.hpp"
#include "printer.hpp"

class Good {
    unsigned int id;
    std::string name;
    double amount;
    std::vector<Material> materials;
    std::string measure;
    bool split;
    double max = 0.;
    unsigned int shoots;
    void removeExcess();

public:
    Good(unsigned int i, const std::string &n, double a, const std::string &m, unsigned int s)
        : id(i), name(n), amount(a), measure(m), split(!m.empty()), shoots(s) {}
    Good(unsigned int i, const std::string &n, const std::string &m, unsigned int s) : Good(i, n, 0., m, s) {}
    Good(unsigned int i, const std::string &n, double a, const std::string &m) : Good(i, n, a, m, 0) {}
    Good(unsigned int i, const std::string &n, const std::string &m) : Good(i, n, 0., m) {}
    Good(unsigned int i, const std::string &n, double a) : Good(i, n, a, "") {}
    Good(unsigned int i, const std::string &n) : Good(i, n, 0) {}
    Good(unsigned int i, double a) : Good(i, "", a) {}
    Good(unsigned int i) : Good(i, "") {}
    Good(const Save::Good *g);
    flatbuffers::Offset<Save::Good> save(flatbuffers::FlatBufferBuilder &b) const;
    bool operator==(const Good &other) const { return id == other.id; }
    bool operator!=(const Good &other) const { return id != other.id; }
    bool operator<(const Good &other) const { return id < other.id; }
    unsigned int getId() const { return id; }
    const std::string &getName() const { return name; }
    std::string getFullName(const Material &m) const;
    const std::vector<Material> &getMaterials() const { return materials; }
    const Material &getMaterial(size_t i) const { return materials[i]; }
    const Material &getMaterial(const Material &m) const;
    const Material &getMaterial(const std::string &mNm) const;
    const Material &getMaterial() const { return materials.front(); }
    double getAmount() const { return amount; }
    const std::string &getMeasure() const { return measure; }
    bool getSplit() const { return split; }
    double getMax() const { return max; }
    unsigned int getShoots() const { return shoots; }
    double getConsumption() const;
    std::string businessText() const;
    std::string logText() const;
    void setAmount(double a);
    void addMaterial(const Material &m);
    void setCombatStats(const std::unordered_map<unsigned int, std::vector<CombatStat>> &cSs);
    void setMax() { max = std::abs(getConsumption()) * Settings::getConsumptionSpaceFactor(); }
    void setMax(double m) {
        if (m > max) max = m;
    }
    void setImage(size_t i, SDL_Surface *img) { materials[i].setImage(img); }
    void setConsumptions(const std::vector<std::array<double, 3>> &cs);
    void scaleConsumptions(unsigned long p);
    void take(Good &g);
    void use(double a);
    void use() { use(amount); }
    void put(Good &g);
    void create(const std::unordered_map<unsigned int, double> &mAs);
    void create(double a);
    void consume(unsigned int e);
    std::unique_ptr<MenuButton> button(bool aS, const Material &mtr, BoxInfo bI, Printer &pr) const;
    std::unique_ptr<MenuButton> button(bool aS, BoxInfo bI, Printer &pr) const {
        return button(aS, materials.front(), bI, pr);
    }
    void adjustDemand(std::string rBN, double d);
    void saveDemand(unsigned long p, std::string &u) const;
};

void createGoodButtons(Pager &pgr, const std::vector<Good> &gds, const SDL_Rect &rt, BoxInfo bI, Printer &pr,
                       const std::function<std::function<void(MenuButton *)>(const Good &, const Material &)> &fn);

#endif // GOOD_H
