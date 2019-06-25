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

#include <unordered_map>

#include "save_generated.h"

#include "material.h"

class Good {
    int id;
    std::string name;
    double amount;
    std::vector<Material> materials;
    double perish;
    double carry;
    std::string measure;
    bool split;
    double max = 0;
    int shoots = 0;
    void removeExcess();

  public:
    Good(int i, const std::string &n, double a, double p, double c, const std::string &m);
    Good(int i, const std::string &n, double p, double c, const std::string &m);
    Good(int i, const std::string &n, double a, const std::string &m);
    Good(int i, const std::string &n, double a);
    Good(int i, const std::string &n);
    Good(int i, double a);
    Good(int i);
    Good(const Save::Good *g);
    bool operator==(const Good &other) const { return id == other.id; }
    bool operator!=(const Good &other) const { return id != other.id; }
    int getId() const { return id; }
    const std::string getName() const { return name; }
    const std::vector<Material> &getMaterials() const { return materials; }
    const Material &getMaterial(const Material &m) const;
    const Material &getMaterial(size_t i) const { return materials[i]; }
    const Material &getMaterial() const { return materials.front(); }
    double getCarry() const { return carry; }
    double getPerish() const { return perish; }
    double getAmount() const { return amount; }
    const std::string &getMeasure() const { return measure; }
    bool getSplit() const { return split; }
    double getMax() const { return max; }
    int getShoots() const { return shoots; }
    double getConsumption() const;
    std::string logEntry() const;
    void setAmount(double a);
    void addMaterial(Material m);
    void setCombatStats(const std::unordered_map<int, std::vector<CombatStat>> &cSs);
    void setMax() { max = std::abs(getConsumption()) * Settings::getConsumptionSpaceFactor(); }
    void setMax(double m) {
        if (m > max)
            max = m;
    }
    void assignConsumption(const std::unordered_map<int, std::array<double, 3>> &cs);
    void assignConsumption(int p);
    void take(Good &g);
    void use(double a);
    void use() { use(amount); }
    void put(Good &g);
    void create(const std::unordered_map<int, double> &mAs);
    void create(double a);
    void consume(int e);
    void adjustDemand(std::string rBN, double d);
    void saveDemand(int p, std::string &u) const;
    flatbuffers::Offset<Save::Good> save(flatbuffers::FlatBufferBuilder &b) const;
};

#endif // GOOD_H
