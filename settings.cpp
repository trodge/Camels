/*
 * This file is part of Camels.
 *
 * Camels is free software: you can redistribute it and/or modify
 * it under the terms of the GNU general Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Camels is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU general Public License for more details.
 *
 * You should have received a copy of the GNU general Public License
 * along with Camels.  If not, see <https://www.gnu.org/licenses/>.
 *
 * © Tom Rodgers notaraptor@gmail.com 2017-2019
 */

#include "settings.hpp"

SDL_Rect Settings::screenRect;
SDL_Rect Settings::mapView;
std::unordered_map<ColorScheme::Scheme, ColorScheme> Settings::colorSchemes;
SDL_Color Settings::routeColor;
SDL_Color Settings::waterColor;
SDL_Color Settings::playerColor;
SDL_Color Settings::aIColor;
int Settings::scroll;
int Settings::offsetX;
int Settings::offsetY;
double Settings::scale;
std::unordered_map<BoxSize::Size, BoxSize> Settings::boxSizes;
int Settings::buttonMargin;
int Settings::goodButtonColumns;
int Settings::goodButtonRows;
int Settings::businessButtonColumns;
int Settings::businessButtonRows;
int Settings::dayLength;
unsigned int Settings::townHeadStart;
int Settings::propertyUpdateTime;
int Settings::travelersCheckTime;
int Settings::aIDecisionTime;
double Settings::consumptionSpaceFactor, Settings::inputSpaceFactor, Settings::outputSpaceFactor;
int Settings::minPriceDivisor;
double Settings::townProfit;
size_t Settings::maxTowns;
size_t Settings::connectionCount;
double Settings::travelersExponent;
int Settings::travelersMin;
unsigned int Settings::statMax;
double Settings::attackDistSq;
double Settings::escapeChance;
std::array<double, 6> Settings::aIRoleWeights;
int Settings::criteriaMax;
unsigned int Settings::aITownRange;
double Settings::limitFactorMin, Settings::limitFactorMax;
double Settings::aIAttackThreshold;
std::mt19937 Settings::rng(std::chrono::high_resolution_clock::now().time_since_epoch().count());

SDL_Rect loadRect(const std::string &n, const SDL_Rect &d, const pt::ptree &t) {
    return {t.get(n + "_x", d.x), t.get(n + "_y", d.y), t.get(n + "_w", d.w), t.get(n + "_h", d.h)};
}

SDL_Color loadColor(const std::string &n, const SDL_Color &d, const pt::ptree &t) {
    return {t.get(n + "_r", d.r), t.get(n + "_g", d.g), t.get(n + "_b", d.b), t.get(n + "_a", d.a)};
}

ColorScheme loadColorScheme(const std::string &n, const ColorScheme &d, const pt::ptree &t) {
    return {loadColor(n + "_foreground", d.foreground, t), loadColor(n + "_background", d.background, t),
            loadColor(n + "_highlight", d.highlight, t)};
}

BoxSize loadBoxSize(const std::string &n, const BoxSize &d, const pt::ptree &t) {
    return {t.get(n + "_border", d.border), t.get(n + "_radius", d.radius), t.get(n + "_fontSize", d.fontSize)};
}

template <class OutputIt, class T>
void loadRange(const std::string &n, OutputIt out, const std::initializer_list<T> &dfs, const pt::ptree &t) {
    auto oIt = out;
    for (auto &df : dfs) {
        *oIt = t.get(n + "_" + std::to_string(oIt - out), df);
        ++oIt;
    }
}

void Settings::load(const fs::path &p) {
    if (!fs::exists(p)) {
        fs::ofstream s(p);
        s.close();
    }
    pt::ptree tree;
    pt::read_ini(p.string(), tree);
    SDL_DisplayMode current;
    SDL_GetCurrentDisplayMode(0, &current);
    screenRect = loadRect("ui.screenRect", {0, 0, current.w * 14 / 15, current.h * 14 / 15}, tree);
    mapView = loadRect("ui.mapView", {3330, 2250, screenRect.w, screenRect.h}, tree);
    colorSchemes.insert({ColorScheme::ui,
                         loadColorScheme("ui.colors", {{0, 0, 0, 255}, {109, 109, 109, 255}, {0, 127, 251, 255}}, tree)});
    colorSchemes.insert(
        {ColorScheme::load, loadColorScheme("ui.loadColors", {{0, 255, 0, 255}, {0, 0, 0, 255}, {0, 0, 0, 0}}, tree)});
    routeColor = loadColor("ui.routeColor", {97, 97, 97, 255}, tree);
    waterColor = loadColor("ui.waterColor", {29, 109, 149, 255}, tree);
    playerColor = loadColor("ui.playerColor", {255, 255, 255, 255}, tree);
    aIColor = loadColor("ui.aIColor", {191, 191, 191, 255}, tree);
    scroll = tree.get("ui.scroll", 64);
    offsetX = tree.get("ui.offsetX", -3197);
    offsetY = tree.get("ui.offsetY", 4731);
    scale = tree.get("ui.scale", 112.5);
    boxSizes.insert({BoxSize::big,
                     loadBoxSize("ui.big", {current.h * 4 / 1080, current.h * 13 / 1080, current.h * 48 / 1080}, tree)});
    boxSizes.insert({BoxSize::load,
                     loadBoxSize("ui.load", {current.h * 4 / 1080, current.h * 13 / 1080, current.h * 36 / 1080}, tree)});
    boxSizes.insert({BoxSize::small,
                     loadBoxSize("ui.small", {current.h * 3 / 1080, current.h * 7 / 1080, current.h * 24 / 1080}, tree)});
    boxSizes.insert({BoxSize::town, loadBoxSize("ui.town", {current.h / 1080, current.h / 1080, current.h * 18 / 1080}, tree)});
    boxSizes.insert({BoxSize::trade,
                     loadBoxSize("ui.trade", {current.h * 2 / 1080, current.h * 5 / 1080, current.h * 16 / 1080}, tree)});
    boxSizes.insert({BoxSize::equip,
                     loadBoxSize("ui.equip", {current.h * 5 / 1080, current.h * 7 / 1080, current.h * 20 / 1080}, tree)});
    boxSizes.insert({BoxSize::fight,
                     loadBoxSize("ui.fight", {current.h * 4 / 1080, current.h * 13 / 1080, current.h * 20 / 1080}, tree)});
    buttonMargin = tree.get("ui.buttonMargin", 3);
    goodButtonColumns = tree.get("ui.goodButtonColumns", 6);
    goodButtonRows = tree.get("ui.goodButtonRows", 11);
    businessButtonColumns = tree.get("ui.businessButtonColumns", 3);
    businessButtonRows = tree.get("ui.businessButtonRows", 3);
    dayLength = tree.get("time.dayLength", 5000);
    townHeadStart = static_cast<unsigned int>(tree.get("time.townHeadStart", 15000));
    propertyUpdateTime = tree.get("time.propertyUpdateTime", 1500);
    travelersCheckTime = tree.get("time.travelersCheckTime", 4500);
    aIDecisionTime = tree.get("time.aIDecisionTime", 12000);
    consumptionSpaceFactor = tree.get("goods.consumptionSpaceFactor", 13.14);
    inputSpaceFactor = tree.get("goods.inputSpaceFactor", 19.71);
    outputSpaceFactor = tree.get("goods.outputSpaceFactor", 19.71);
    minPriceDivisor = tree.get("goods.minPriceDivisor", 2047);
    townProfit = tree.get("towns.profit", 0.9);
    maxTowns = static_cast<size_t>(tree.get("towns.max", 500));
    connectionCount = static_cast<size_t>(tree.get("towns.connectionCount", 5));
    travelersExponent = tree.get("travelers.exponent", 0.24);
    travelersMin = tree.get("travelers.min", -13);
    statMax = static_cast<unsigned int>(tree.get("travelers.statMax", 15));
    attackDistSq = tree.get("travelers.attackDistSq", 9000);
    escapeChance = tree.get("travelers.escapeChance", 0.5);
    criteriaMax = tree.get("aI.criteriaMax", 9);
    loadRange("aI.typeWeights", begin(aIRoleWeights), {0.4, 0.1, 0.1, 0.2, 0.1, 0.1}, tree);
    aITownRange = static_cast<unsigned int>(tree.get("aI.townRange", 9));
    limitFactorMin = tree.get("aI.limitFactorMin", 0.1);
    limitFactorMax = tree.get("aI.limitFactorMax", 0.9);
    aIAttackThreshold = tree.get("aI.attackThreshold", 13500);
}

void saveRect(const std::string &n, const SDL_Rect &r, pt::ptree &t) {
    t.put(n + "_x", r.x);
    t.put(n + "_y", r.y);
    t.put(n + "_w", r.w);
    t.put(n + "_h", r.h);
}

void saveColor(const std::string &n, const SDL_Color &c, pt::ptree &t) {
    t.put(n + "_r", c.r);
    t.put(n + "_g", c.g);
    t.put(n + "_b", c.b);
}

void saveColorScheme(const std::string &n, const ColorScheme &s, pt::ptree &t) {
    saveColor(n + "_foreground", s.foreground, t), saveColor(n + "_background", s.background, t),
        saveColor(n + "_highlight", s.highlight, t);
}

void saveBoxSize(const std::string &n, const BoxSize &s, pt::ptree &t) {
    t.put(n + "_border", s.border);
    t.put(n + "_radius", s.radius);
    t.put(n + "_fontSize", s.fontSize);
}

template <class InputIt> void saveRange(const std::string &n, InputIt bgn, InputIt end, pt::ptree &t) {
    for (auto it = bgn; it != end; ++it) t.put(n + "_" + std::to_string(it - bgn), *it);
}

void Settings::save(const fs::path &p) {
    pt::ptree tree;
    saveRect("ui.mapView", mapView, tree);
    saveColorScheme("ui.colors", colorSchemes[ColorScheme::ui], tree);
    saveColorScheme("ui.loadColors", colorSchemes[ColorScheme::load], tree);
    saveColor("ui.routeColor", routeColor, tree);
    saveColor("ui.waterColor", waterColor, tree);
    saveColor("ui.playerColor", playerColor, tree);
    saveColor("ui.aIColor", aIColor, tree);
    tree.put("ui.scroll", scroll);
    tree.put("ui.offsetX", offsetX);
    tree.put("ui.offsetY", offsetY);
    tree.put("ui.scale", scale);
    saveBoxSize("ui.big", boxSizes[BoxSize::big], tree);
    saveBoxSize("ui.load", boxSizes[BoxSize::load], tree);
    saveBoxSize("ui.small", boxSizes[BoxSize::small], tree);
    saveBoxSize("ui.town", boxSizes[BoxSize::town], tree);
    saveBoxSize("ui.trade", boxSizes[BoxSize::trade], tree);
    saveBoxSize("ui.equip", boxSizes[BoxSize::equip], tree);
    saveBoxSize("ui.fight", boxSizes[BoxSize::fight], tree);
    tree.put("ui.buttonMargin", buttonMargin);
    tree.put("ui.goodButtonColumns", goodButtonColumns);
    tree.put("ui.goodButtonRows", goodButtonRows);
    tree.put("ui.businessButtonColumns", businessButtonColumns);
    tree.put("ui.businessButtonRows", businessButtonRows);
    tree.put("time.dayLength", dayLength);
    tree.put("time.townHeadStart", townHeadStart);
    tree.put("time.propertyUpdateTime", propertyUpdateTime);
    tree.put("time.travelersCheckTime", travelersCheckTime);
    tree.put("time.aIDecisionTime", aIDecisionTime);
    tree.put("goods.consumptionSpaceFactor", consumptionSpaceFactor);
    tree.put("goods.inputSpaceFactor", inputSpaceFactor);
    tree.put("goods.outputSpaceFactor", outputSpaceFactor);
    tree.put("goods.minPriceDivisor", minPriceDivisor);
    tree.put("towns.profit", townProfit);
    tree.put("towns.max", maxTowns);
    tree.put("towns.connectionCount", connectionCount);
    tree.put("travelers.exponent", travelersExponent);
    tree.put("travelers.min", travelersMin);
    tree.put("travelers.statMax", statMax);
    tree.put("travelers.attackDistSq", attackDistSq);
    tree.put("travelers.escapeChance", escapeChance);
    tree.put("aI.criteriaMax", criteriaMax);
    saveRange("aI.typeWeights", begin(aIRoleWeights), end(aIRoleWeights), tree);
    tree.put("aI.townRange", aITownRange);
    tree.put("aI.limitFactorMin", limitFactorMin);
    tree.put("aI.limitFactorMax", limitFactorMax);
    tree.put("aI.attackThreshold", aIAttackThreshold);
    pt::write_ini(p.string(), tree);
}

double Settings::random() {
    static std::uniform_real_distribution<double> rDis(0, 1);
    return rDis(rng);
}

int Settings::propertyUpdateCounter() {
    // Randomize business run counter.
    static std::uniform_int_distribution<> uCDis(-propertyUpdateTime, 0);
    return uCDis(rng);
}

int Settings::travelerCount(unsigned long ppl) {
    // Randomize traveler count in town with given population.
    std::uniform_int_distribution<> tCDis(travelersMin, static_cast<int>(pow(ppl, travelersExponent)));
    int n = tCDis(rng);
    return std::max(n, 0);
}

int Settings::travelersCheckCounter() {
    // Randomize travelers check counter.
    static std::uniform_int_distribution<> tCTDis(-travelersCheckTime, 0);
    return tCTDis(rng);
}

std::array<unsigned int, kStatCount> Settings::travelerStats() {
    // Randomize traveler stats.
    static std::uniform_int_distribution<unsigned int> stDis(1, statMax);
    std::array<unsigned int, kStatCount> stats;
    for (auto &stat : stats) stat = stDis(rng);
    return stats;
}

int Settings::aIRole() {
    // Randomize ai role.
    static std::discrete_distribution<> rlDis(begin(aIRoleWeights), end(aIRoleWeights));
    return rlDis(rng);
}

std::array<double, kDecisionCriteriaCount> Settings::aIDecisionCriteria() {
    // Randomize decision criteria.
    static std::uniform_real_distribution<double> dcCrtDis(1, criteriaMax);
    std::array<double, kDecisionCriteriaCount> dcCrt;
    for (auto &dC : dcCrt) dC = dcCrtDis(rng);
    return dcCrt;
}

double Settings::aIDecisionCounter() {
    // Randomize decision counter.
    static std::uniform_int_distribution<> dcCntDis(-aIDecisionTime, 0);
    return dcCntDis(rng);
}

double Settings::aILimitFactor() {
    static std::uniform_real_distribution<double> lFDis(limitFactorMin, limitFactorMax);
    return lFDis(rng);
}

BoxInfo Settings::boxInfo(const SDL_Rect &rt, const std::vector<std::string> &tx, ColorScheme sm, unsigned int i,
                          bool iN, bool cF, bool cE, BoxSize::Size sz, SDL_Keycode ky,
                          const std::function<void(MenuButton *)> &fn, bool scl, const SDL_Rect &oR) {
    return {rt, tx, sm, i, iN, cF, cE, boxSizes[sz], {}, ky, fn, scl, oR};
}
