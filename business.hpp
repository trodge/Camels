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

#ifndef BUSINESS_H
#define BUSINESS_H

#include <iostream>
#include <string>
#include <unordered_map>
#include <vector>

#include "constants.hpp"
#include "good.hpp"
#include "property.hpp"

class Property;
class Good;

struct Conflict;

class Business {
    unsigned int id, mode;
    std::string name;
    double area;    // in uncia
    bool canSwitch; // whether able to switch between modes
    bool requireCoast;
    bool keepMaterial;                            // whether outputs get input as material
    std::vector<Good> requirements;               // goods needed to start
    std::vector<Good> reclaimables;               // goods which will be reclaimed when business is demolished
    std::vector<Good> inputs;                     // goods needed every production cycle
    std::vector<Good> outputs;                    // goods created every production cycle
    double factor;                                // factor based on area and available inputs for production
    double frequency = 0;                         // area of business per unit of population
    EnumArray<double, TownType> frequencyFactors; // factors for frequency in different town types
    double reclaimFactor = 0.7;                   // portion of requirements that can be reclaimed

public:
    Business(unsigned int i, unsigned int m, const std::string &nm, bool cS, bool rC, bool kM,
             const EnumArray<double, TownType> &fFs);
    Business(const Business &bsn, unsigned int m)
        : Business(bsn.id, m, bsn.name, bsn.canSwitch, bsn.requireCoast, bsn.keepMaterial, bsn.frequencyFactors) {}
    Business(const Save::Business *svBsn);
    flatbuffers::Offset<Save::Business> save(flatbuffers::FlatBufferBuilder &b) const;
    bool operator==(const Business &other) const { return (id == other.id && mode == other.mode); }
    bool operator!=(const Business &other) const { return (id != other.id || mode != other.mode); }
    bool operator<(const Business &other) const {
        return (id < other.id || (id == other.id && mode < other.mode));
    }
    unsigned int getId() const { return id; }
    unsigned int getMode() const { return mode; }
    const std::string &getName() const { return name; }
    double getArea() const { return area; }
    bool getCanSwitch() const { return canSwitch; }
    bool getRequireCoast() const { return requireCoast; }
    bool getKeepMaterial() const { return keepMaterial; }
    const std::vector<Good> &getRequirements() const { return requirements; }
    const std::vector<Good> &getInputs() const { return inputs; }
    const std::vector<Good> &getOutputs() const { return outputs; }
    double getFrequency() const { return frequency; }
    void setArea(double a);
    void changeArea(double a) { setArea(area + a); }
    void scale(unsigned long ppl, TownType tT);
    void setRequirements(const std::vector<Good> &rqs) { requirements = rqs; }
    void setInputs(const std::vector<Good> &ips) { inputs = ips; }
    void setOutputs(const std::vector<Good> &ops) { outputs = ops; }
    void setFactor(double ft) { factor = ft; }
    void setFactor(double ft, const Property &inv, std::unordered_map<unsigned int, Conflict> &cfcts);
    void setFrequency(double fq) { frequency = fq; }
    void takeRequirements(Property &inv, double a);
    void reclaim(Property &inv, double a);
    void run(Property &inv, const std::unordered_map<unsigned int, Conflict> &cfcts);
    std::unique_ptr<MenuButton> button(bool aS, BoxInfo &bI, Printer &pr) const;
    void saveFrequency(unsigned long p, std::string &u) const;
};

struct BusinessPlan {
    const Business &business;
    double factor, cost, profit;
    std::vector<Good> request;
    bool build;
};

#endif // BUSINESS_H
