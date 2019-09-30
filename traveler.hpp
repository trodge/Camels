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
#include <memory>
#include <string>
#include <unordered_set>
#include <vector>

#include <SDL2/SDL.h>

#include "ai.hpp"
#include "constants.hpp"
#include "draw.hpp"
#include "enum_array.hpp"
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
    EnumArray<std::string, Part> partNames;
    EnumArray<std::string, Status> statusNames;
    EnumArray<CombatOdd, AttackType> odds;
    EnumArray<std::string, TownType> townTypeNames;
    std::map<unsigned long, std::string> populationAdjectives;
};

struct CombatHit {
    double time;
    const CombatOdd *odd;
    std::string weapon;
};

enum class FightChoice { none = -1, fight, run, yield, count };

class AI;

struct Contract;

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
    EnumArray<unsigned int, Stat> stats;
    EnumArray<Status, Part> parts;
    std::vector<Good> equipment;
    std::unordered_multimap<AIRole, Traveler *> employees; // travelers employed by role
    std::unique_ptr<Contract> contract;                    // contract with employer, if any
    std::unordered_set<Traveler *> enemies, allies;        // travelers currently fighting
    Traveler *target = nullptr;                            // current target for attacks
    unsigned int targeterCount;                            // number of enemies targeting this traveler
    std::unique_ptr<CombatHit> nextHit;                    // next hit on target
    double fightTime;                                      // time left to fight this round
    FightChoice choice;
    bool dead = false; // true if traveler is not alive and not being looted from
    std::unique_ptr<AI> aI;
    const GameData &gameData;
    std::forward_list<Town *> pathTo(const Town *t) const;
    int pathDistSq(const Town *t) const;
    Property &makeProperty(unsigned int tId);
    void insertAllies(AIRole rl);
    void insertAllies(const std::vector<AIRole> &rls);

public:
    Traveler(const std::string &n, Town *t, const GameData &gD);
    Traveler(const Save::Traveler *ldTvl, const std::vector<Nation> &nts, std::vector<Town> &tns, const GameData &gD);
    flatbuffers::Offset<Save::Traveler> save(flatbuffers::FlatBufferBuilder &b) const;
    std::string getName() const { return name; }
    const Town *town() const { return toTown; }
    const Nation *getNation() const { return nation; }
    const std::vector<std::string> &getLogText() const { return logText; }
    const Property &property() const { return properties.find(0)->second; }
    const Property *property(unsigned int tId) const {
        auto pptIt = properties.find(tId);
        return pptIt == end(properties) ? nullptr : &pptIt->second;
    }
    const std::vector<Good> &getOffer() const { return offer; }
    const std::vector<Good> &getRequest() const { return request; }
    double getPortion() const { return portion; }
    const EnumArray<unsigned int, Stat> &getStats() const { return stats; }
    double speed() const { return stats[Stat::strength] + stats[Stat::endurance] + stats[Stat::agility]; }
    Status part(Part pt) const { return parts[pt]; }
    const std::vector<Good> &getEquipment() const { return equipment; }
    Traveler *getTarget() const { return target; }
    unsigned int getTargeterCount() const { return targeterCount; }
    const std::unordered_set<Traveler *> &getAllies() const { return allies; }
    bool alive() const {
        return static_cast<unsigned int>(parts[Part::head]) < 5 && static_cast<unsigned int>(parts[Part::torso]) < 5;
    }
    bool getDead() const { return dead; }
    bool getMoving() const { return moving; }
    const Position &getPosition() const { return position; }
    double weight() const {
        return properties.find(0)->second.weight() + static_cast<double>(stats[Stat::strength]) * kTravelerCarry;
    }
    bool fightWon() const { return target && (target->choice == FightChoice::yield || !target->alive()); }
    void choose(FightChoice c) { choice = c; }
    void setPortion(double p);
    void changePortion(double d);
    void addToTown();
    void create(unsigned int fId, double amt);
    void pickTown(const Town *tn);
    void place(const SDL_Point &ofs, double s) { position.place(ofs, s); }
    void draw(SDL_Renderer *s) const;
    void clearTrade();
    void offerGood(Good &&gd) { offer.push_back(gd); }
    void requestGood(Good &&gd) { request.push_back(gd); }
    void requestGoods(std::vector<Good> &&gds) { request = std::move(gds); }
    void updatePortionBox(TextBox *bx) const;
    void divideExcess(double exc, double tnP);
    void makeTrade();
    BoxInfo boxInfo(const SDL_Rect &rt, const std::vector<std::string> &tx, BoxSizeType sz, BoxBehavior bvr,
                    SDL_Keycode ky, const std::function<void(MenuButton *)> &fn) const {
        return Settings::boxInfo(rt, tx, nation->getColors(), {0, false}, sz, bvr, ky, fn);
    }
    BoxInfo boxInfo(const SDL_Rect &rt, const std::vector<std::string> &tx, BoxSizeType sz, BoxBehavior bvr) const {
        return boxInfo(rt, tx, sz, bvr, SDLK_UNKNOWN, nullptr);
    }
    BoxInfo boxInfo(const SDL_Rect &rt, const std::vector<std::string> &tx, BoxSizeType sz) const {
        return boxInfo(rt, tx, sz, BoxBehavior::inert);
    } // all traveler boxes
    BoxInfo boxInfo(const SDL_Rect &rt, const std::vector<std::string> &tx) const {
        return boxInfo(rt, tx, BoxSizeType::fight);
    } // fight boxes
    BoxInfo boxInfo() const {
        return boxInfo({0, 0, 0, 0}, {}, BoxSizeType::trade, BoxBehavior::focus);
    } // good and business buttons
    void updateTradeButtons(std::vector<Pager> &pgrs);
    void deposit(Good &g);
    void withdraw(Good &g);
    void build(const Business &bsn, double a);
    void demolish(const Business &bsn, double a);
    void unequip(Part pt);
    void equip(Good &g);
    void equip(Part pt);
    void bid(double bns, double wge);
    void hire(size_t idx);
    std::vector<Traveler *> attackable() const;
    void attack(Traveler *tgt);
    void createLogBox(Pager &pgr, Printer &pr);
    void disengage();
    void setTarget(const std::unordered_set<Traveler *> &enms);
    CombatHit firstHit();
    void setNextHit();
    void useAmmo(double tm);
    void forAlly(const std::function<void(Traveler *)> &fn);
    void forCombatant(const std::function<void(Traveler *)> &fn);
    void fight(unsigned int elTm);
    void hit();
    void createFightBoxes(Pager &pgr, bool &p, Printer &pr);
    void updateFightBoxes(Pager &pgr);
    void loot(Good &g);
    void loot();
    void createAIGoods(AIRole rl);
    void startAI();
    void startAI(const Traveler &p);
    void update(unsigned int e);
    void toggleMaxGoods();
    void resetTown();
    void adjustAreas(Pager &pgr, double mM);
    void adjustDemand(Pager &pgr, double mM);
};

struct Contract {
    Traveler *party; // the other party to this contract, or this if contract is a bid
    double owed;     // value holder will retain at end of contract
    double wage;     // value holder earns daily, in deniers
};

template <class Source, class Destination>
void transfer(std::vector<Good> &gds, Source &src, Destination &dst, std::string &lgEt);

#endif // TRAVELER_H
