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
};

enum class ColorSchemeType { ui, load, count };

struct BoxSize {
    int border = 0, radius = 0, fontSize = 0;
};

enum class BoxSizeType { big, load, small, town, trade, equip, fight, count };

struct BoxInfo {
    SDL_Rect rect{0, 0, 0, 0};
    std::vector<std::string> text;
    ColorScheme colors;
    unsigned int id = 0;
    bool isNation = false, canFocus = false, canEdit = false;
    BoxSize size;
    std::vector<Image> images;
    SDL_Keycode key = SDLK_UNKNOWN;
    std::function<void(MenuButton *)> onClick = nullptr;
    bool scroll = false;
    SDL_Rect outsideRect{0, 0, 0, 0};
};

enum class TownType { city, town, fort, count };

enum class Stat { strength, endurance, agility, intelligence, charisma, count };

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
    static std::array<double, static_cast<size_t>(AIRole::count)> aIRoleWeights;
    static int criteriaMax;
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
    static int getCriteriaMax() { return criteriaMax; }
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
    static AIRole aIRole();
    static std::array<double, kDecisionCriteriaCount> aIDecisionCriteria();
    static double aIDecisionCounter();
    static double aILimitFactor();
    static BoxSize boxSize(BoxSizeType sz) { return boxSizes[static_cast<size_t>(sz)]; }
    static BoxInfo boxInfo(const SDL_Rect &rt, const std::vector<std::string> &tx, ColorScheme sm,
                           unsigned int i, bool iN, bool cF, bool cE, BoxSizeType sz, SDL_Keycode ky,
                           const std::function<void(MenuButton *)> &fn, bool scl,
                           const SDL_Rect &oR); // any box
    static BoxInfo boxInfo(const SDL_Rect &rt, const std::vector<std::string> &tx, ColorScheme sm,
                           unsigned int i, bool iN, BoxSizeType sz, SDL_Keycode ky,
                           const std::function<void(MenuButton *)> &fn, bool scl) {
        return boxInfo(rt, tx, sm, i, iN, true, false, sz, ky, fn, scl, {0, 0, 0, 0});
    } // any button
    static BoxInfo boxInfo(const SDL_Rect &rt, const std::vector<std::string> &tx, ColorScheme sm, BoxSizeType sz,
                           SDL_Keycode ky, const std::function<void(MenuButton *)> &fn, bool scl) {
        return boxInfo(rt, tx, sm, 0, false, sz, ky, fn, scl);
    } // select button with scheme
    static BoxInfo boxInfo(const SDL_Rect &rt, const std::vector<std::string> &tx, ColorScheme sm, unsigned int i,
                           bool iN, BoxSizeType sz, SDL_Keycode ky, const std::function<void(MenuButton *)> &fn) {
        return boxInfo(rt, tx, sm, i, iN, sz, ky, fn, false);
    } // button with color scheme and id
    static BoxInfo boxInfo(const SDL_Rect &rt, const std::vector<std::string> &tx, ColorScheme sm,
                           BoxSizeType sz, SDL_Keycode ky, const std::function<void(MenuButton *)> &fn) {
        return boxInfo(rt, tx, sm, 0, false, sz, ky, fn, false);
    } // button with color scheme
    static BoxInfo boxInfo(const SDL_Rect &rt, const std::vector<std::string> &tx, BoxSizeType sz,
                           SDL_Keycode ky, const std::function<void(MenuButton *)> &fn, bool scl) {
        return boxInfo(rt, tx, colorSchemes[static_cast<size_t>(ColorSchemeType::ui)], 0, false, sz, ky, fn, scl);
    } // ui select button
    static BoxInfo boxInfo(const SDL_Rect &rt, const std::vector<std::string> &tx, BoxSizeType sz,
                           SDL_Keycode ky, const std::function<void(MenuButton *)> &fn) {
        return boxInfo(rt, tx, sz, ky, fn, false);
    } // ui menu button
    static BoxInfo boxInfo(const SDL_Rect &rt, const std::vector<std::string> &tx, ColorScheme sm, unsigned int i,
                           bool iN, bool cF, bool cE, BoxSizeType sz, bool scl, const SDL_Rect &oR) {
        return boxInfo(rt, tx, sm, i, iN, cF, cE, sz, SDLK_UNKNOWN, nullptr, scl, oR);
    } // any text box
    static BoxInfo boxInfo(const SDL_Rect &rt, const std::vector<std::string> &tx, const SDL_Rect &oR) {
        return boxInfo(rt, tx, colorSchemes[static_cast<size_t>(ColorSchemeType::load)], 0, false, false, false, BoxSizeType::load, false, oR);
    } // load bar
    static BoxInfo boxInfo(const SDL_Rect &rt, const std::vector<std::string> &tx, ColorScheme sm,
                           unsigned int i, bool iN, bool cF, bool cE, BoxSizeType sz, bool scl) {
        return boxInfo(rt, tx, sm, i, iN, cF, cE, sz, scl, {0, 0, 0, 0});
    } // any non-load bar text box
    static BoxInfo boxInfo(const SDL_Rect &rt, const std::vector<std::string> &tx, ColorScheme sm, bool cE,
                           BoxSizeType sz, bool scl) {
        return boxInfo(rt, tx, sm, 0, false, true, cE, sz, scl);
    } // scroll box with color scheme
    static BoxInfo boxInfo(const SDL_Rect &rt, const std::vector<std::string> &tx, ColorScheme sm,
                           unsigned int i, bool iN, bool cF, bool cE, BoxSizeType sz) {
        return boxInfo(rt, tx, sm, i, iN, cF, cE, sz, false);
    } // box with color scheme and id
    static BoxInfo boxInfo(const SDL_Rect &rt, const std::vector<std::string> &tx, ColorScheme sm, bool cF,
                           bool cE, BoxSizeType sz) {
        return boxInfo(rt, tx, sm, 0, false, cF, cE, sz);
    } // box with color scheme
    static BoxInfo boxInfo(const SDL_Rect &rt, const std::vector<std::string> &tx, ColorScheme sm, BoxSizeType sz) {
        return boxInfo(rt, tx, sm, 0, false, false, false, sz);
    } // box with color scheme
    static BoxInfo boxInfo(const SDL_Rect &rt, const std::vector<std::string> &tx, bool cE, BoxSizeType sz) {
        return boxInfo(rt, tx, colorSchemes[static_cast<size_t>(ColorSchemeType::ui)], cE, cE, sz);
    } // editable ui box
    static BoxInfo boxInfo(const SDL_Rect &rt, const std::vector<std::string> &tx, BoxSizeType sz) {
        return boxInfo(rt, tx, false, sz);
    } // ui box
};

#endif // SETTINGS_H
