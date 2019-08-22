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

#ifndef SETTINGS_H
#define SETTINGS_H
#include <array>
#include <chrono>
#include <random>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include <boost/filesystem.hpp>
#include <boost/property_tree/ini_parser.hpp>
#include <boost/property_tree/ptree.hpp>
namespace fs = boost::filesystem;
namespace pt = boost::property_tree;
#include <SDL2/SDL.h>

#include "constants.hpp"

class MenuButton;

struct Image {
    SDL_Surface *surface;
    SDL_Rect rect;
};

struct ColorScheme {
    SDL_Color foreground{0, 0, 0, 0}, background{0, 0, 0, 0}, highlight{0, 0, 0, 0};
    bool invert = false;
};

enum class ColorSchemeType { ui, load, count };

struct BoxSize {
    int border = 0, radius = 0, fontSize = 0;
};

enum class BoxSizeType { big, load, small, town, trade, equip, fight, count };

enum class BoxBehavior { inert, focus, edit, scroll };

struct BoxInfo {
    SDL_Rect rect{0, 0, 0, 0};
    std::vector<std::string> text;
    ColorScheme colors;
    std::pair<unsigned int, bool> id = {0, false};
    BoxSize size;
    BoxBehavior behavior; // behavoir when clicked
    std::vector<Image> images;
    SDL_Keycode key = SDLK_UNKNOWN;
    std::function<void(MenuButton *)> onClick = nullptr;
    SDL_Rect outsideRect{0, 0, 0, 0};
};

enum class TownType { city, town, fort, count };

enum class Part { head, torso, leftArm, rightArm, leftLeg, rightLeg, count };

enum class Stat { strength, endurance, agility, intelligence, charisma, count };

enum class AttackType { none = -1, bash, slash, stab, count };

enum class DecisionCriteria {
    buyScoreWeight,     // buy score weight in deciding where to go
    sellScoreWeight,    // sell score weight in deciding where to go
    attackScoreWeight,  // attack weight for equip scores
    defenseScoreWeight, // defense weight for equip scores
    equipScoreWeight,   // equip score weight for buying goods
    fightTendency,      // start fights and fight back when attacked
    runTendency,        // run away when attacked
    yieldTendency,      // yield when attacked
    lootingGreed,       // portion of goods to take on yield
    count
};

enum class AIRole { trader, soldier, bandit, agent, guard, thug, count };

class Settings {
    static SDL_Rect screenRect;
    static SDL_Rect mapView;
    static std::array<ColorScheme, static_cast<size_t>(ColorSchemeType::count)> colorSchemes;
    static SDL_Color routeColor;
    static SDL_Color waterColor;
    static SDL_Color playerColor;
    static SDL_Color aIColor;
    static int scroll, offsetX, offsetY;
    static double scale;
    static std::array<BoxSize, static_cast<size_t>(BoxSizeType::count)> boxSizes;
    static int buttonMargin;           // margin between good and business buttons in pixels
    static int goodButtonColumns;      // number of columns of good buttons
    static int goodButtonRows;         // number of rows of good buttons
    static int businessButtonColumns;  // number of columns of business buttons
    static int businessButtonRows;     // number of rows of business buttons
    static int dayLength;              // length of a day in milliseconds
    static unsigned int townHeadStart; // number of milliseconds to run before game starts on new game
    static int propertyUpdateTime;     // time between business cycles in milliseconds
    static int travelersCheckTime;     // time between checks of dead travelers in milliseconds
    static int aIDecisionTime;         // time between AI cycles in milliseconds
    static double consumptionSpaceFactor, inputSpaceFactor, outputSpaceFactor;
    static int minPriceDivisor;
    static double townProfit;
    static size_t maxTowns;        // max number of towns to load from database
    static size_t connectionCount; // number of connections each town makes
    static double travelersExponent;
    static int travelersMin;
    static unsigned int statMax;
    static double attackDistSq;
    static double escapeChance;
    static std::vector<std::pair<unsigned int, double>> playerStartingGoods;
    static std::array<double, static_cast<size_t>(AIRole::count)> aIRoleWeights;
    static std::array<std::vector<std::pair<unsigned int, double>>, static_cast<size_t>(AIRole::count)> aIStartingGoods;
    static std::array<unsigned int, static_cast<size_t>(AIRole::count)> aIStartingGoodsCount;
    static int aIDecisionCriteriaMax;
    static unsigned int aITownRange;
    static double limitFactorMin, limitFactorMax;
    static double aIAttackThreshold;
    static std::mt19937 rng;

public:
    static void load(const fs::path &p);
    static void save(const fs::path &p);
    static const SDL_Rect &getScreenRect() { return screenRect; }
    static const SDL_Rect &getMapView() { return mapView; }
    static const SDL_Color &getRouteColor() { return routeColor; }
    static const SDL_Color &getWaterColor() { return waterColor; }
    static const SDL_Color &getPlayerColor() { return playerColor; }
    static const SDL_Color &getAIColor() { return aIColor; }
    static int getScroll() { return scroll; }
    static int getOffsetX() { return offsetX; }
    static int getOffsetY() { return offsetY; }
    static double getScale() { return scale; }
    static int getButtonMargin() { return buttonMargin; }
    static int getGoodButtonColumns() { return goodButtonColumns; }
    static int getGoodButtonRows() { return goodButtonRows; }
    static int getBusinessButtonColumns() { return businessButtonColumns; }
    static int getBusinessButtonRows() { return businessButtonRows; }
    static int getDayLength() { return dayLength; }
    static unsigned int getTownHeadStart() { return townHeadStart; }
    static int getPropertyUpdateTime() { return propertyUpdateTime; }
    static int getTravelersCheckTime() { return travelersCheckTime; }
    static int getAIDecisionTime() { return aIDecisionTime; }
    static double getConsumptionSpaceFactor() { return consumptionSpaceFactor; }
    static double getInputSpaceFactor() { return inputSpaceFactor; }
    static double getOutputSpaceFactor() { return outputSpaceFactor; }
    static int getMinPriceDivisor() { return minPriceDivisor; }
    static double getTownProfit() { return townProfit; }
    static size_t getMaxTowns() { return maxTowns; }
    static size_t getMaxNeighbors() { return connectionCount; }
    static double getTravelersExponent() { return travelersExponent; }
    static int getTravelersMin() { return travelersMin; }
    static unsigned int getStatMax() { return statMax; }
    static double getAttackDistSq() { return attackDistSq; }
    static double getEscapeChance() { return escapeChance; }
    static const std::vector<std::pair<unsigned int, double>> &getPlayerStartingGoods() {
        return playerStartingGoods;
    }
    static int getAIDecisionCriteriaMax() { return aIDecisionCriteriaMax; }
    static unsigned int getAITownRange() { return aITownRange; }
    static double getAIAttackThreshold() { return aIAttackThreshold; }
    template <typename T> static T randomInt(T max) {
        // Return a random int less than or equal to max.
        std::uniform_int_distribution<T> iDis(0, max);
        return iDis(rng);
    }
    template <typename T> static auto &randomChoice(const std::vector<T> &options) {
        return options[randomInt(options.size() - 1)];
    }
    static double random();
    static int propertyUpdateCounter();
    static int travelerCount(unsigned long ppl);
    static int travelersCheckCounter();
    static std::array<unsigned int, static_cast<size_t>(Stat::count)> travelerStats();
    static Part part();
    static AIRole aIRole();
    static std::vector<std::pair<unsigned int, double>> getAIStartingGoods(AIRole rl);
    static std::array<double, static_cast<size_t>(DecisionCriteria::count)> aIDecisionCriteria();
    static double aIDecisionCounter();
    static double aILimitFactor();
    static BoxSize boxSize(BoxSizeType sz) { return boxSizes[static_cast<size_t>(sz)]; }
    static BoxInfo boxInfo(const SDL_Rect &rt, const std::vector<std::string> &tx, ColorScheme sm,
                           std::pair<unsigned int, bool> i, BoxSizeType sz, BoxBehavior bvr, SDL_Keycode ky,
                           const std::function<void(MenuButton *)> &fn,
                           const SDL_Rect &oR); // any text box
    static BoxInfo boxInfo(const SDL_Rect &rt, const std::vector<std::string> &tx, ColorScheme sm,
                           std::pair<unsigned int, bool> i, BoxSizeType sz, BoxBehavior bvr, SDL_Keycode ky,
                           const std::function<void(MenuButton *)> &fn) {
        return boxInfo(rt, tx, sm, i, sz, bvr, ky, fn, {0, 0, 0, 0});
    } // any button
    static BoxInfo boxInfo(const SDL_Rect &rt, const std::vector<std::string> &tx, ColorScheme sm, BoxSizeType sz,
                           BoxBehavior bvr, SDL_Keycode ky, const std::function<void(MenuButton *)> &fn) {
        return boxInfo(rt, tx, sm, {0, false}, sz, bvr, ky, fn);
    } // select button with scheme
    static BoxInfo boxInfo(const SDL_Rect &rt, const std::vector<std::string> &tx, ColorScheme sm,
                           std::pair<unsigned int, bool> i, BoxSizeType sz, SDL_Keycode ky,
                           const std::function<void(MenuButton *)> &fn) {
        return boxInfo(rt, tx, sm, i, sz, BoxBehavior::focus, ky, fn);
    } // button with color scheme and id
    static BoxInfo boxInfo(const SDL_Rect &rt, const std::vector<std::string> &tx, ColorScheme sm,
                           BoxSizeType sz, SDL_Keycode ky, const std::function<void(MenuButton *)> &fn) {
        return boxInfo(rt, tx, sm, {0, false}, sz, ky, fn);
    } // button with color scheme
    static BoxInfo boxInfo(const SDL_Rect &rt, const std::vector<std::string> &tx, BoxSizeType sz,
                           BoxBehavior bvr, SDL_Keycode ky, const std::function<void(MenuButton *)> &fn) {
        return boxInfo(rt, tx, colorSchemes[static_cast<size_t>(ColorSchemeType::ui)], {0, false}, sz, bvr, ky, fn);
    } // ui select button
    static BoxInfo boxInfo(const SDL_Rect &rt, const std::vector<std::string> &tx, BoxSizeType sz,
                           SDL_Keycode ky, const std::function<void(MenuButton *)> &fn) {
        return boxInfo(rt, tx, sz, BoxBehavior::focus, ky, fn);
    } // ui menu button
    static BoxInfo boxInfo(const SDL_Rect &rt, const std::vector<std::string> &tx, ColorScheme sm,
                           std::pair<unsigned int, bool> i, BoxSizeType sz, BoxBehavior bvr, const SDL_Rect &oR) {
        return boxInfo(rt, tx, sm, i, sz, bvr, SDLK_UNKNOWN, nullptr, oR);
    } // any non-button box
    static BoxInfo boxInfo(const SDL_Rect &rt, const std::vector<std::string> &tx, const SDL_Rect &oR) {
        return boxInfo(rt, tx, colorSchemes[static_cast<size_t>(ColorSchemeType::load)], {0, false},
                       BoxSizeType::load, BoxBehavior::inert, oR);
    } // load bar
    static BoxInfo boxInfo(const SDL_Rect &rt, const std::vector<std::string> &tx, ColorScheme sm,
                           std::pair<unsigned int, bool> i, BoxSizeType sz, BoxBehavior bvr) {
        return boxInfo(rt, tx, sm, i, sz, bvr, {0, 0, 0, 0});
    } // any non-load bar text box
    static BoxInfo boxInfo(const SDL_Rect &rt, const std::vector<std::string> &tx, ColorScheme sm,
                           BoxSizeType sz, BoxBehavior bvr) {
        return boxInfo(rt, tx, sm, {0, false}, sz, bvr);
    } // scroll or edit box with color scheme
    static BoxInfo boxInfo(const SDL_Rect &rt, const std::vector<std::string> &tx, ColorScheme sm,
                           std::pair<unsigned int, bool> i, BoxSizeType sz) {
        return boxInfo(rt, tx, sm, i, sz, BoxBehavior::inert);
    } // box with color scheme and id
    static BoxInfo boxInfo(const SDL_Rect &rt, const std::vector<std::string> &tx, ColorScheme sm, BoxSizeType sz) {
        return boxInfo(rt, tx, sm, {0, false}, sz);
    } // box with color scheme
    static BoxInfo boxInfo(const SDL_Rect &rt, const std::vector<std::string> &tx, BoxSizeType sz, BoxBehavior bvr) {
        return boxInfo(rt, tx, colorSchemes[static_cast<size_t>(ColorSchemeType::ui)], sz, bvr);
    } // edit or scroll ui box
    static BoxInfo boxInfo(const SDL_Rect &rt, const std::vector<std::string> &tx, BoxSizeType sz) {
        return boxInfo(rt, tx, sz, BoxBehavior::inert);
    } // ui box
};

#endif // SETTINGS_H
