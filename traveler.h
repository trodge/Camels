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

#ifndef TRAVELER_H
#define TRAVELER_H

#include <algorithm>
#include <array>
#include <cmath>
#include <forward_list>
#include <functional>
#include <limits>
#include <set>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include <SDL2/SDL.h>

#include "ai.h"
#include "scrollbox.h"
#include "town.h"

struct CombatOdd {
    double hitOdds;
    std::array<std::pair<unsigned int, double>, 3> statusChances;
    // statuses are normal, bruised, wounded, broken, infected, pulverized, amputated, impaled
};

struct GameData {
    std::vector<std::string> parts;
    std::vector<std::string> statuses;
    std::vector<CombatOdd> odds;
    std::vector<std::string> townTypeNouns;
    std::map<int, std::string> populationAdjectives;
};

struct CombatHit {
    double time;
    unsigned int partId, status;
    std::string weapon;
};

enum class FightChoice { none = -1, fight, run, yield };

class AI;

class Town;

class Business;

class Traveler : public std::enable_shared_from_this<Traveler> {
    std::string name;
    Town *toTown, *fromTown;
    const Nation *nation;
    std::vector<std::string> logText;
    double longitude, latitude;
    std::vector<Good> goods;          // goods carried by traveler
    std::vector<Good> offer, request; // goods offered and requested in next trade
    size_t requestButtonIndex;        // index of request and offer button for updating trade buttons
    double tradePortion;              // portion of goods offered in next trade
    std::weak_ptr<Traveler> target;   // pointer to enemy if currently fighting
    double fightTime; // time left to fight this round
    std::array<int, 5> stats; // strength, endurance, agility, intelligence, charisma
    std::array<int, 6> parts; // head, torso, left arm, right arm, left leg, right leg
    std::vector<Good> equipment;
    const GameData *gameData;
    std::unordered_map<Town *, std::vector<Business>> businesses; // businesses owned by this traveler in given towns
    std::unordered_map<Town *, std::vector<Good>> storage;        // goods stored by this traveler in given towns
    std::unique_ptr<AI> ai;
    bool moving;
    int px, py;
    double netWeight() const;
    std::forward_list<Town *> pathTo(const Town *t) const;
    double distSq(int x, int y) const;
    double pathDist(const Town *t) const;
    CombatHit firstHit(Traveler &t, std::uniform_real_distribution<> &d);
    void useAmmo(double t);
    void runFight(Traveler &t, int e, std::uniform_real_distribution<> &d);
    void takeHit(const CombatHit &cH, Traveler &t);

  public:
    Traveler(const std::string &n, Town *t, const GameData *gD);
    Traveler(const Save::Traveler *t, std::vector<Town> &ts, const std::vector<Nation> &ns, const GameData *gD);
    std::string getName() const { return name; }
    const Town *getTown() const { return toTown; }
    const Nation *getNation() const { return nation; }
    const std::vector<std::string> &getLogText() const { return logText; }
    const std::vector<Good> &getGoods() const { return goods; }
    const Good &getGood(int i) const { return goods[i]; }
    const std::vector<Good> &getEquipment() const { return equipment; }
    const std::array<int, 5> &getStats() const { return stats; }
    int speed() const { return stats[1] + stats[2] + stats[3]; }
    double getTradePortion() const { return tradePortion; }
    const std::weak_ptr<Traveler> getTarget() const { return target; }
    bool alive() const { return parts[0] < 5 and parts[1] < 5; }
    int getPX() const { return px; }
    int getPY() const { return py; }
    FightChoice choice;
    bool fightWon() const {
        auto t = target.lock();
        return t and (t->choice == FightChoice::yield or not t->alive());
    }
    double getFightTime() const { return fightTime; }
    std::string tradePortionString() const;
    void setRequestButtonIndex(size_t i) { requestButtonIndex = i; }
    void setTradePortion(double p) { tradePortion = p; }
    void changeTradePortion(double d);
    void addToTown();
    void pickTown(const Town *t);
    void place(int ox, int oy, double s);
    void draw(SDL_Surface *s) const;
    void updateTradeButtons(std::vector<std::unique_ptr<TextBox>> &bs);
    void makeTrade();
    void unequip(int pI);
    void equip(Good &g);
    void equip(int pI);
    std::vector<std::shared_ptr<Traveler>> attackable() const;
    void attack(std::shared_ptr<Traveler> t);
    void loseTarget();
    void updateFightBoxes(std::vector<std::unique_ptr<TextBox>> &bs);
    void loot(Good &g, Traveler &t);
    void loot(Traveler &t) {
        for (auto g : t.goods)
            loot(g, t);
    }
    void leave(Good &g, Traveler &t) { t.loot(g, *this); }
    void startAI();
    void startAI(const Traveler &p);
    void runAI(int e);
    void update(int e);
    void adjustAreas(const std::vector<std::unique_ptr<TextBox>> &bs, double d);
    void adjustDemand(const std::vector<std::unique_ptr<TextBox>> &bs, double d);
    void resetTown();
    void toggleMaxGoods();
    flatbuffers::Offset<Save::Traveler> save(flatbuffers::FlatBufferBuilder &b);
};

#endif // TRAVELER_H
