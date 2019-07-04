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

#include <sqlite3.h>

#include "good.hpp"

class Good;

class Business {
    unsigned int id, mode;
    std::string name;
    double area;    // in uncia
    bool canSwitch; // whether able to switch between modes
    bool requireCoast;
    bool keepMaterial;              // whether outputs get input as material
    std::vector<Good> requirements; // goods needed to start
    std::vector<Good> inputs;       // goods needed every production cycle
    std::vector<Good> outputs;      // goods created every production cycle
    double factor;                  // factor based on area and available inputs for production
    double frequencyFactor;         // area of business per unit of population

  public:
    Business(unsigned int i, unsigned int m, sqlite3 *c);
    Business(const Save::Business *b);
    bool operator==(const Business &other) const;
    unsigned int getId() const { return id; }
    unsigned int getMode() const { return mode; }
    double getArea() const { return area; }
    bool getCanSwitch() const { return canSwitch; }
    bool getRequireCoast() const { return requireCoast; }
    bool getKeepMaterial() const { return keepMaterial; }
    const std::vector<Good> &getInputs() const { return inputs; }
    const std::vector<Good> &getOutputs() const { return outputs; }
    void run(std::vector<Good> &gs);
    void setArea(double a);
    void setFactor(double f) { factor = f; }
    void setFrequencyFactor(double f) {
        frequencyFactor = f;
        area *= f;
    }
    void addConflicts(std::vector<int> &cs, std::vector<Good> &gs);
    void handleConflicts(std::vector<int> &cs);
    void saveFrequency(unsigned long p, std::string &u) const;
    flatbuffers::Offset<Save::Business> save(flatbuffers::FlatBufferBuilder &b) const;
};

#endif // BUSINESS_H
