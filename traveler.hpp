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
#include <string>
#include <unordered_set>
#include <vector>

#include <SDL2/SDL.h>

#include "ai.hpp"
#include "good.hpp"
#include "menubutton.hpp"
#include "nation.hpp"
#include "pager.hpp"
#include "scrollbox.hpp"
#include "selectbutton.hpp"
#include "textbox.hpp"
#include "town.hpp"

struct CombatOdd {
    double hitOdds;
    std::array<std::pair<unsigned int, double>, 3> statusChances;
    // statuses are normal, bruised, wounded, broken, infected, pulverized, amputated, impaled
};

struct GameData {
    unsigned int townCount;
    std::vector<std::string> parts;
    std::vector<std::string> statuses;
    std::vector<CombatOdd> odds;
    std::vector<std::string> townTypeNouns;
    std::map<unsigned long, std::string> populationAdjectives;
};

struct CombatHit {
    double time;
    unsigned int partId, status;
    std::string weapon;
};

struct Property {
    std::vector<Good> storage;
    std::vector<Business> businesses;
};

enum class FightChoice { none = -1, fight, run, yield };

class AI;

class Traveler : public std::enable_shared_from_this<Traveler> {
    std::string name;
    Town *toTown, *fromTown;
    const Nation *nation;
    std::vector<std::string> logText;
    double longitude, latitude;
    bool moving;
    int px, py;
    double portion;                    // portion of goods offered in next trade
    std::vector<Good> goods;           // goods carried by traveler
    std::vector<Good> offer, request;  // goods offered and requested in next trade
    std::vector<Property> properties;  // vector of owned goods and businesses indexed by town id
    std::array<unsigned int, 5> stats; // strength, endurance, agility, intelligence, charisma
    std::array<unsigned int, 6> parts; // head, torso, left arm, right arm, left leg, right leg
    std::vector<Good> equipment;
    std::weak_ptr<Traveler> target; // pointer to enemy if currently fighting
    double fightTime;               // time left to fight this round
    FightChoice choice;
    std::unique_ptr<AI> aI;
    const GameData &gameData;
    std::forward_list<Town *> pathTo(const Town *t) const;
    double distSq(int x, int y) const;
    double pathDist(const Town *t) const;
    void deposit(Good &g);
    void withdraw(Good &g);
    void refreshFocusBox(std::vector<Pager> &pgrs, int &fB);
    void refreshStorageButtons(std::vector<Pager> &pgrs, int &fB, Printer &pr);
    void build(const Business &bsn, double a);
    void demolish(const Business &bsn, double a);
    void refreshEquipButtons(std::vector<Pager> &pgrs, int &fB, Printer &pr);
    CombatHit firstHit(Traveler &t, std::uniform_real_distribution<> &d);
    void useAmmo(double t);
    void runFight(Traveler &t, unsigned int e, std::uniform_real_distribution<> &d);
    void takeHit(const CombatHit &cH, Traveler &t);
    void refreshLootButtons(std::vector<Pager> &pgrs, int &fB, Printer &pr);

  public:
    Traveler(const std::string &n, Town *t, const GameData &gD);
    Traveler(const Save::Traveler *t, std::vector<Town> &ts, const std::vector<Nation> &ns, const GameData &gD);
    std::string getName() const { return name; }
    const Town *getTown() const { return toTown; }
    const Nation *getNation() const { return nation; }
    const std::vector<std::string> &getLogText() const { return logText; }
    const std::vector<Good> &getGoods() const { return goods; }
    const std::vector<Good> &getOffer() const { return offer; }
    const std::vector<Good> &getRequest() const { return request; }
    const Good &getGood(unsigned int i) const { return goods[i]; }
    double getPortion() const { return portion; }
    const std::array<unsigned int, 5> &getStats() const { return stats; }
    double getSpeed() const { return stats[1] + stats[2] + stats[3]; }
    unsigned int getPart(size_t i) const { return parts[i]; }
    const std::vector<Good> &getEquipment() const { return equipment; }
    std::weak_ptr<Traveler> getTarget() const { return target; }
    bool alive() const { return parts[0] < 5 && parts[1] < 5; }
    bool getMoving() const { return moving; }
    int getPX() const { return px; }
    int getPY() const { return py; }
    bool fightWon() const {
        auto t = target.lock();
        return t && (t->choice == FightChoice::yield || !t->alive());
    }
    double getFightTime() const { return fightTime; }
    FightChoice getChoice() const { return choice; }
    void setPortion(double p);
    void changePortion(double d);
    void addToTown();
    double netWeight() const;
    void pickTown(const Town *t);
    void place(int ox, int oy, double s);
    void draw(SDL_Renderer *s) const;
    void updatePortionBox(TextBox *bx) const;
    void clearTrade();
    void offerGood(const Good &g) { offer.push_back(g); };
    void requestGood(const Good &g) { request.push_back(g); }
    void divideExcess(double ec);
    void makeTrade();
    void createTradeButtons(std::vector<Pager> &pgrs, Printer &pr);
    void updateTradeButtons(std::vector<Pager> &pgrs);
    void createStorageButtons(std::vector<Pager> &pgrs, int &fB, Printer &pr);
    void createManageButtons(std::vector<Pager> &pgrs, Printer &pr);
    void unequip(unsigned int pI);
    void equip(Good &g);
    void equip(unsigned int pI);
    void createEquipButtons(std::vector<Pager> &pgrs, int &fB, Printer &pr);
    std::vector<std::shared_ptr<Traveler>> attackable() const;
    void attack(std::shared_ptr<Traveler> t);
    void createAttackButton(Pager &pgr, std::function<void()> sSF, Printer &pr);
    void choose(FightChoice c) { choice = c; }
    void loseTarget();
    void createFightBoxes(Pager &pgr, bool &p, Printer &pr);
    void updateFightBoxes(Pager &pgr);
    void loot(Good &g, Traveler &t);
    void loot(Traveler &t);
    void createLootButtons(std::vector<Pager> &pgrs, int &fB, Printer &pr);
    void startAI();
    void startAI(const Traveler &p);
    void runAI(unsigned int e);
    void update(unsigned int e);
    void adjustAreas(Pager &pgr, double mM);
    void adjustDemand(Pager &pgr, double mM);
    void resetTown();
    void toggleMaxGoods();
    flatbuffers::Offset<Save::Traveler> save(flatbuffers::FlatBufferBuilder &b) const;
};

#endif // TRAVELER_H
