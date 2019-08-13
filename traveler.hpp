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
#include "constants.hpp"
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

struct Contract {
    Traveler *party; // the other party to this contract, or this if contract is a bid
    double owed;     // value of goods holder will retain at end of contract
    double wage;     // value holder earns daily, in deniers
};

enum class FightChoice { none = -1, fight, run, yield };

class AI;

class Traveler {
    std::string name;
    Town *toTown, *fromTown;
    const Nation *nation;
    std::unordered_map<unsigned int, int> reputation; // reputation mapped by nation id
    std::vector<std::string> logText;
    double longitude, latitude;
    bool moving;
    int px, py;
    double portion;                                        // portion of goods offered in next trade
    std::vector<Good> offer, request;                      // goods offered and requested in next trade
    std::unordered_map<unsigned int, Property> properties; // owned goods and businesses by town id, 0 for carried goods
    std::array<unsigned int, 5> stats;                     // strength, endurance, agility, intelligence, charisma
    std::array<unsigned int, 6> parts;                     // head, torso, left arm, right arm, left leg, right leg
    std::vector<Good> equipment;
    std::vector<Traveler *> agents;     // travelers employed
    std::unique_ptr<Contract> contract; // contract with employer, if any
    Traveler *target = nullptr;         // pointer to enemy if currently fighting
    double fightTime;                   // time left to fight this round
    FightChoice choice;
    bool dead = false; // true if traveler is not alive and not being looted from
    std::unique_ptr<AI> aI;
    const GameData &gameData;
    std::forward_list<Town *> pathTo(const Town *t) const;
    double distSq(int x, int y) const;
    double pathDist(const Town *t) const;
    BoxInfo boxInfo(const SDL_Rect &rt, const std::vector<std::string> &tx, BoxSize::Size sz, SDL_Keycode ky,
                    const std::function<void(MenuButton *)> &fn, bool scl) {
        return Settings::boxInfo(rt, tx, nation->getColors(), 0, false, sz, ky, fn, scl);
    }
    BoxInfo boxInfo(const SDL_Rect &rt, const std::vector<std::string> &tx, BoxSize::Size sz, bool scl) {
        return boxInfo(rt, tx, sz, SDLK_UNKNOWN, nullptr, scl);
    }
    BoxInfo boxInfo(const SDL_Rect &rt, const std::vector<std::string> &tx, BoxSize::Size sz) {
        return boxInfo(rt, tx, sz, false);
    }
    BoxInfo boxInfo(const SDL_Rect &rt, const std::vector<std::string> &tx) {
        return boxInfo(rt, tx, BoxSize::trade);
    } // fight boxes
    BoxInfo boxInfo() { return boxInfo({0, 0, 0, 0}, {}); }
    void refreshFocusBox(std::vector<Pager> &pgrs, int &fB);
    void refreshStorageButtons(std::vector<Pager> &pgrs, int &fB, Printer &pr);
    void refreshBuildButtons(std::vector<Pager> &pgrs, int &fB, Printer &pr);
    void refreshEquipButtons(std::vector<Pager> &pgrs, int &fB, Printer &pr);
    void refreshLootButtons(std::vector<Pager> &pgrs, int &fB, Printer &pr);

public:
    Traveler(const std::string &n, Town *t, const GameData &gD);
    Traveler(const Save::Traveler *t, std::vector<Town> &ts, const std::vector<Nation> &ns, const GameData &gD);
    flatbuffers::Offset<Save::Traveler> save(flatbuffers::FlatBufferBuilder &b) const;
    std::string getName() const { return name; }
    const Town *getTown() const { return toTown; }
    const Nation *getNation() const { return nation; }
    const std::vector<std::string> &getLogText() const { return logText; }
    const Property &getProperty() const { return properties.find(0)->second; }
    const std::vector<Good> &getOffer() const { return offer; }
    const std::vector<Good> &getRequest() const { return request; }
    double getPortion() const { return portion; }
    const std::array<unsigned int, 5> &getStats() const { return stats; }
    double speed() const { return stats[1] + stats[2] + stats[3]; }
    unsigned int getPart(size_t i) const { return parts[i]; }
    const std::vector<Good> &getEquipment() const { return equipment; }
    Traveler *getTarget() const { return target; }
    bool alive() const { return parts[0] < 5 && parts[1] < 5; }
    bool getDead() const { return dead; }
    bool getMoving() const { return moving; }
    int getPX() const { return px; }
    int getPY() const { return py; }
    double weight() const;
    bool fightWon() const { return target && (target->choice == FightChoice::yield || !target->alive()); }
    double getFightTime() const { return fightTime; }
    FightChoice getChoice() const { return choice; }
    void setPortion(double p);
    void changePortion(double d);
    void addToTown();
    void pickTown(const Town *t);
    void place(int ox, int oy, double s);
    void draw(SDL_Renderer *s) const;
    void clearTrade();
    void offerGood(Good &&gd) { offer.push_back(gd); }
    void requestGood(Good &&gd) { request.push_back(gd); }
    void updatePortionBox(TextBox *bx) const;
    void divideExcess(double ec);
    void makeTrade();
    void createTradeButtons(std::vector<Pager> &pgrs, Printer &pr);
    void updateTradeButtons(std::vector<Pager> &pgrs);
    Property &property(unsigned int tId);
    void deposit(Good &g);
    void withdraw(Good &g);
    void createStorageButtons(std::vector<Pager> &pgrs, int &fB, Printer &pr);
    void build(const Business &bsn, double a);
    void demolish(const Business &bsn, double a);
    void createBuildButtons(std::vector<Pager> &pgrs, int &fB, Printer &pr);
    void unequip(unsigned int pI);
    void equip(Good &g);
    void equip(unsigned int pI);
    void createEquipButtons(std::vector<Pager> &pgrs, int &fB, Printer &pr);
    void createManageButtons(std::vector<Pager> &pgrs, int &fB, Printer &pr);
    std::vector<Traveler *> attackable() const;
    void attack(Traveler *t);
    void createAttackButton(Pager &pgr, std::function<void()> sSF, Printer &pr);
    void createLogBox(Pager &pgr, Printer &pr);
    void loseTarget();
    CombatHit firstHit(Traveler &t, std::uniform_real_distribution<double> &d);
    void useAmmo(double t);
    void fight(Traveler &t, unsigned int e, std::uniform_real_distribution<double> &d);
    void takeHit(const CombatHit &cH, Traveler &t);
    void createFightBoxes(Pager &pgr, bool &p, Printer &pr);
    void updateFightBoxes(Pager &pgr);
    void loot(Good &g);
    void loot();
    void createLootButtons(std::vector<Pager> &pgrs, int &fB, Printer &pr);
    void startAI();
    void startAI(const Traveler &p);
    void update(unsigned int e);
    void toggleMaxGoods();
    void resetTown();
    void adjustAreas(Pager &pgr, double mM);
    void adjustDemand(Pager &pgr, double mM);
    friend class AI;
    friend class Player;
};

#endif // TRAVELER_H
