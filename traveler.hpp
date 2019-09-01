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
#include "draw.hpp"
#include "constants.hpp"
#include "good.hpp"
#include "menubutton.hpp"
#include "nation.hpp"
#include "pager.hpp"
#include "scrollbox.hpp"
#include "selectbutton.hpp"
#include "settings.hpp"
#include "textbox.hpp"
#include "town.hpp"

enum class Status { normal, bruised, wounded, broken, infected, pulverized, amputated, impaled, count };

struct CombatOdd {
    double hitChance;
    std::array<std::pair<Status, double>, kStatusChanceCount> statusChances;
};

struct GameData {
    unsigned int nationCount, townCount;
    std::array<std::string, static_cast<size_t>(Part::count)> partNames;
    std::array<std::string, static_cast<size_t>(Status::count)> statusNames;
    std::array<CombatOdd, static_cast<size_t>(AttackType::count)> odds;
    std::array<std::string, static_cast<size_t>(TownType::count)> townTypeNames;
    std::map<unsigned long, std::string> populationAdjectives;
};

struct Contract {
    Traveler *party; // the other party to this contract, or this if contract is a bid
    double owed;     // value of goods holder will retain at end of contract
    double wage;     // value holder earns daily, in deniers
};

struct CombatHit {
    double time;
    const CombatOdd *odd;
    std::string weapon;
};

enum class FightChoice { none = -1, fight, run, yield };

class AI;

class Traveler {
    std::string name;
    const Nation *nation;
    Town *toTown, *fromTown;
    std::vector<std::string> logText;
    Position position; // coordinates of traveler in world
    bool moving;
    double portion;                                        // portion of goods offered in next trade
    std::vector<int> reputation;                           // reputation indexed by nation id
    std::vector<Good> offer, request;                      // goods offered and requested in next trade
    std::unordered_map<unsigned int, Property> properties; // owned goods and businesses by town id, 0 for carried goods
    std::array<unsigned int, static_cast<size_t>(Stat::count)> stats;
    std::array<Status, static_cast<size_t>(Part::count)> parts;
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
    int pathDistSq(const Town *t) const;
    BoxInfo boxInfo(const SDL_Rect &rt, const std::vector<std::string> &tx, BoxSizeType sz, BoxBehavior bvr,
                    SDL_Keycode ky, const std::function<void(MenuButton *)> &fn) {
        return Settings::boxInfo(rt, tx, nation->getColors(), {0, false}, sz, bvr, ky, fn);
    }
    BoxInfo boxInfo(const SDL_Rect &rt, const std::vector<std::string> &tx, BoxSizeType sz, BoxBehavior bvr) {
        return boxInfo(rt, tx, sz, bvr, SDLK_UNKNOWN, nullptr);
    }
    BoxInfo boxInfo(const SDL_Rect &rt, const std::vector<std::string> &tx, BoxSizeType sz) {
        return boxInfo(rt, tx, sz, BoxBehavior::inert);
    } // all traveler boxes
    BoxInfo boxInfo(const SDL_Rect &rt, const std::vector<std::string> &tx) {
        return boxInfo(rt, tx, BoxSizeType::fight);
    } // fight boxes
    BoxInfo boxInfo() {
        return boxInfo({0, 0, 0, 0}, {}, BoxSizeType::trade, BoxBehavior::focus);
    } // good and business buttons
    Property &property(unsigned int tId);
    void refreshFocusBox(std::vector<Pager> &pgrs, int &fB);
    void refreshStorageButtons(std::vector<Pager> &pgrs, int &fB, Printer &pr);
    void refreshBuildButtons(std::vector<Pager> &pgrs, int &fB, Printer &pr);
    void refreshEquipButtons(std::vector<Pager> &pgrs, int &fB, Printer &pr);
    void refreshLootButtons(std::vector<Pager> &pgrs, int &fB, Printer &pr);
    void createAIGoods();

public:
    Traveler(const std::string &n, Town *t, const GameData &gD);
    Traveler(const Save::Traveler *ldTvl, const std::vector<Nation> &nts, std::vector<Town> &tns, const GameData &gD);
    flatbuffers::Offset<Save::Traveler> save(flatbuffers::FlatBufferBuilder &b) const;
    std::string getName() const { return name; }
    const Town *town() const { return toTown; }
    const Nation *getNation() const { return nation; }
    const std::vector<std::string> &getLogText() const { return logText; }
    const Property &property() const { return properties.find(0)->second; }
    const std::vector<Good> &getOffer() const { return offer; }
    const std::vector<Good> &getRequest() const { return request; }
    double getPortion() const { return portion; }
    const std::array<unsigned int, 5> &getStats() const { return stats; }
    double speed() const { return stats[1] + stats[2] + stats[3]; }
    Status part(Part pt) const { return parts[static_cast<size_t>(pt)]; }
    const std::vector<Good> &getEquipment() const { return equipment; }
    Traveler *getTarget() const { return target; }
    bool alive() const {
        return static_cast<unsigned int>(parts[0]) < 5 && static_cast<unsigned int>(parts[1]) < 5;
    }
    bool getDead() const { return dead; }
    bool getMoving() const { return moving; }
    const Position &getPosition() const { return position; }
    double weight() const;
    bool fightWon() const { return target && (target->choice == FightChoice::yield || !target->alive()); }
    void choose(FightChoice c) { choice = c; }
    void setPortion(double p);
    void changePortion(double d);
    void addToTown();
    void create(unsigned int gId, double amt);
    void pickTown(const Town *tn);
    void place(const SDL_Point &ofs, double s) { position.place(ofs, s); }
    void draw(SDL_Renderer *s) const;
    void clearTrade();
    void offerGood(Good &&gd) { offer.push_back(gd); }
    void requestGood(Good &&gd) { request.push_back(gd); }
    void requestGoods(std::vector<Good> &&gds) { request = std::move(gds); }
    void updatePortionBox(TextBox *bx) const;
    void divideExcess(double exc);
    void makeTrade();
    void createTradeButtons(std::vector<Pager> &pgrs, Printer &pr);
    void updateTradeButtons(std::vector<Pager> &pgrs);
    void deposit(Good &g);
    void withdraw(Good &g);
    void createStorageButtons(std::vector<Pager> &pgrs, int &fB, Printer &pr);
    void build(const Business &bsn, double a);
    void demolish(const Business &bsn, double a);
    void createBuildButtons(std::vector<Pager> &pgrs, int &fB, Printer &pr);
    void unequip(Part pt);
    void equip(Good &g);
    void equip(Part pt);
    void createEquipButtons(std::vector<Pager> &pgrs, int &fB, Printer &pr);
    void createManageButtons(std::vector<Pager> &pgrs, int &fB, Printer &pr);
    std::vector<Traveler *> attackable() const;
    void attack(Traveler *tgt);
    void createAttackButton(Pager &pgr, std::function<void()> sSF, Printer &pr);
    void createLogBox(Pager &pgr, Printer &pr);
    void loseTarget();
    CombatHit firstHit(Traveler &tgt);
    void useAmmo(double t);
    void fight(Traveler &tgt, unsigned int elTm);
    void takeHit(const CombatHit &cH, Traveler &tgt);
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
};

#endif // TRAVELER_H
