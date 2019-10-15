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

SDL_Rect Settings::screenRect, Settings::mapView;
EnumArray<ColorScheme, ColorSchemeType> Settings::colorSchemes;
SDL_Color Settings::routeColor, Settings::waterColor;
int Settings::scroll;
SDL_Point Settings::offset;
double Settings::scale;
EnumArray<BoxSize, BoxSizeType> Settings::boxSizes;
int Settings::buttonMargin, Settings::goodButtonColumns, Settings::goodButtonRows,
    Settings::businessButtonColumns, Settings::businessButtonRows, Settings::dayLength;
unsigned int Settings::townHeadStart;
int Settings::propertyUpdateTime, Settings::travelersCheckTime, Settings::aIDecisionTime, Settings::aIBusinessInterval;
double Settings::consumptionSpaceFactor, Settings::inputSpaceFactor, Settings::outputSpaceFactor;
int Settings::minPriceDivisor;
double Settings::townProfit;
size_t Settings::maxTowns, Settings::connectionCount;
double Settings::travelersExponent;
int Settings::travelersMin;
unsigned int Settings::statMax;
int Settings::attackDistSq;
double Settings::escapeChance;
std::vector<std::pair<unsigned int, double>> Settings::playerStartingGoods;
SDL_Color Settings::playerColor;
EnumArray<double, AIRole> Settings::aIRoleWeights;
EnumArray<AIStartingGoods, AIRole> Settings::aIStartingGoods;
int Settings::aIDecisionCriteriaMax;
unsigned int Settings::aITownRange, Settings::aIGoodsCount;
double Settings::aILimitFactorMin, Settings::aILimitFactorMax, Settings::aIAttackThreshold;
SDL_Color Settings::aIColor;
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

template <typename First, typename Second>
std::pair<First, Second> loadPair(const std::string &n, const std::pair<First, Second> &df, const pt::ptree &t) {
    return std::make_pair(t.get(n + "_first", df.first), t.get(n + "_second", df.second));
}

template <class OutputIt, typename T, typename Function>
void loadRange(const std::string &n, OutputIt out, const Function &fn, const std::vector<T> &dfs, const pt::ptree &t) {
    unsigned int i = 0;
    std::transform(begin(dfs), end(dfs), out,
                   [&n, &i, &fn, &t](auto &df) { return fn(n + "_" + std::to_string(++i), df, t); });
}

template <class OutputIt, typename T>
void loadRange(const std::string &n, OutputIt out, const std::vector<T> &dfs, const pt::ptree &t) {
    loadRange(
        n, out, [](const std::string &m, const T &df, const pt::ptree &r) { return r.get(m, df); }, dfs, t);
}

AIStartingGoods loadAIStartingGoods(const std::string &n, const AIStartingGoods &d, const pt::ptree &t) {
    AIStartingGoods goods{t.get(n + "_count", d.count)};
    loadRange(n + "_goods", std::back_inserter(goods.goods), &loadPair<unsigned int, double>, d.goods, t);
    return goods;
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
    colorSchemes[ColorSchemeType::ui] =
        loadColorScheme("ui.colors", {{0, 0, 0, 255}, {109, 109, 109, 255}, {0, 127, 251, 255}}, tree);
    colorSchemes[ColorSchemeType::load] =
        loadColorScheme("ui.loadColors", {{0, 255, 0, 255}, {0, 0, 0, 255}, {0, 0, 0, 0}}, tree);
    routeColor = loadColor("ui.routeColor", {97, 97, 97, 255}, tree);
    waterColor = loadColor("ui.waterColor", {29, 109, 149, 255}, tree);
    scroll = tree.get("ui.scroll", 64);
    offset.x = tree.get("ui.offsetX", -3197);
    offset.y = tree.get("ui.offsetY", 4731);
    scale = tree.get("ui.scale", 112.5);
    boxSizes[BoxSizeType::big] =
        loadBoxSize("ui.big", {current.h * 4 / 1080, current.h * 13 / 1080, current.h * 48 / 1080}, tree);
    boxSizes[BoxSizeType::load] =
        loadBoxSize("ui.load", {current.h * 4 / 1080, current.h * 13 / 1080, current.h * 36 / 1080}, tree);
    boxSizes[BoxSizeType::small] =
        loadBoxSize("ui.small", {current.h * 3 / 1080, current.h * 7 / 1080, current.h * 24 / 1080}, tree);
    boxSizes[BoxSizeType::town] = loadBoxSize(
        "ui.town", {std::max(current.h / 1080, 1), std::max(current.h / 1080, 1), current.h * 18 / 1080}, tree);
    boxSizes[BoxSizeType::trade] =
        loadBoxSize("ui.trade", {current.h * 2 / 1080, current.h * 5 / 1080, current.h * 16 / 1080}, tree);
    boxSizes[BoxSizeType::equip] =
        loadBoxSize("ui.equip", {current.h * 5 / 1080, current.h * 7 / 1080, current.h * 20 / 1080}, tree);
    boxSizes[BoxSizeType::fight] =
        loadBoxSize("ui.fight", {current.h * 4 / 1080, current.h * 13 / 1080, current.h * 20 / 1080}, tree);
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
    aIBusinessInterval = tree.get("time.aIBusinessInterval", 20);
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
    loadRange("player.startingGoods", std::back_inserter(playerStartingGoods), &loadPair<unsigned int, double>,
              std::vector<std::pair<unsigned int, double>>{{96, 0.75}, {100, 2.}}, tree);
    playerColor = loadColor("player.color", {255, 255, 255, 255}, tree);
    aIDecisionCriteriaMax = tree.get("aI.decisionCriteriaMax", 9);
    loadRange("aI.roleWeights", begin(aIRoleWeights), std::vector<double>{0.4, 0.1, 0.1, 0.2, 0.1, 0.1}, tree);
    aIStartingGoods[AIRole::trader] =
        loadAIStartingGoods("aI.traderGoods", {2, {{1, 21.}, {2, 23.}, {16, 10.5}, {96, 0.75}, {100, 2.}}}, tree);
    aIStartingGoods[AIRole::soldier] = loadAIStartingGoods(
        "aI.soldierGoods", {3, {{69, 1.}, {73, 1.}, {77, 1.}, {79, 1.}, {81, 1.}, {87, 1.}}}, tree);
    aIStartingGoods[AIRole::bandit] = loadAIStartingGoods(
        "aI.banditGoods", {2, {{67, 1.}, {70, 1.}, {78, 1.}, {80, 1.}, {82, 1.}, {86, 1.}}}, tree);
    aIStartingGoods[AIRole::agent] = loadAIStartingGoods("aI.agentGoods", {}, tree);
    aIStartingGoods[AIRole::guard] = loadAIStartingGoods("aI.guardGoods", {}, tree);
    aIStartingGoods[AIRole::thug] = loadAIStartingGoods("aI.thugGoods", {}, tree);
    aITownRange = static_cast<unsigned int>(tree.get("aI.townRange", 5));
    aIGoodsCount = static_cast<unsigned int>(tree.get("aI.goodsCount", 13));
    aILimitFactorMin = tree.get("aI.limitFactorMin", 0.1);
    aILimitFactorMax = tree.get("aI.limitFactorMax", 0.9);
    aIAttackThreshold = tree.get("aI.attackThreshold", 13500);
    aIColor = loadColor("ai.color", {191, 191, 191, 255}, tree);
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

template <typename First, typename Second>
void savePair(const std::string &n, const std::pair<First, Second> &pr, pt::ptree &t) {
    t.put(n + "_first", pr.first);
    t.put(n + "_second", pr.second);
}

template <class InputIt, typename Function>
void saveRange(const std::string &n, InputIt bgn, InputIt end, const Function &fn, pt::ptree &t) {
    unsigned int i = 0;
    std::for_each(bgn, end, [&n, &i, &fn, &t](auto &a) { fn(n + "_" + std::to_string(++i), a, t); });
}

template <class InputIt> void saveRange(const std::string &n, InputIt bgn, InputIt end, pt::ptree &t) {
    saveRange(
        n, bgn, end, [](const std::string &m, const decltype(*bgn) &sv, pt::ptree &r) { r.put(m, sv); }, t);
}

void saveAIStartingGoods(const std::string &n, const AIStartingGoods &s, pt::ptree &t) {
    t.put(n + "_count", s.count);
    saveRange(n + "_goods", begin(s.goods), end(s.goods), &savePair<unsigned int, double>, t);
}

void Settings::save(const fs::path &p) {
    pt::ptree tree;
    saveRect("ui.mapView", mapView, tree);
    saveColorScheme("ui.colors", colorSchemes[ColorSchemeType::ui], tree);
    saveColorScheme("ui.loadColors", colorSchemes[ColorSchemeType::load], tree);
    saveColor("ui.routeColor", routeColor, tree);
    saveColor("ui.waterColor", waterColor, tree);
    tree.put("ui.scroll", scroll);
    tree.put("ui.offsetX", offset.x);
    tree.put("ui.offsetY", offset.y);
    tree.put("ui.scale", scale);
    saveBoxSize("ui.big", boxSizes[BoxSizeType::big], tree);
    saveBoxSize("ui.load", boxSizes[BoxSizeType::load], tree);
    saveBoxSize("ui.small", boxSizes[BoxSizeType::small], tree);
    saveBoxSize("ui.town", boxSizes[BoxSizeType::town], tree);
    saveBoxSize("ui.trade", boxSizes[BoxSizeType::trade], tree);
    saveBoxSize("ui.equip", boxSizes[BoxSizeType::equip], tree);
    saveBoxSize("ui.fight", boxSizes[BoxSizeType::fight], tree);
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
    tree.put("time.aIBusinessInterval", aIBusinessInterval);
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
    saveRange("player.startingGoods", begin(playerStartingGoods), end(playerStartingGoods),
              &savePair<unsigned int, double>, tree);
    saveColor("player.color", playerColor, tree);
    tree.put("aI.decisionCriteriaMax", aIDecisionCriteriaMax);
    saveRange("aI.roleWeights", begin(aIRoleWeights), end(aIRoleWeights), tree);
    saveAIStartingGoods("aI.traderGoods", aIStartingGoods[AIRole::trader], tree);
    saveAIStartingGoods("aI.soldierGoods", aIStartingGoods[AIRole::soldier], tree);
    saveAIStartingGoods("aI.banditGoods", aIStartingGoods[AIRole::bandit], tree);
    saveAIStartingGoods("aI.agentGoods", aIStartingGoods[AIRole::agent], tree);
    saveAIStartingGoods("aI.guardGoods", aIStartingGoods[AIRole::guard], tree);
    saveAIStartingGoods("aI.thugGoods", aIStartingGoods[AIRole::thug], tree);
    tree.put("aI.townRange", aITownRange);
    tree.put("aI.goodsCount", aIGoodsCount);
    tree.put("aI.limitFactorMin", aILimitFactorMin);
    tree.put("aI.limitFactorMax", aILimitFactorMax);
    tree.put("aI.attackThreshold", aIAttackThreshold);
    saveColor("ui.aIColor", aIColor, tree);
    pt::write_ini(p.string(), tree);
}

template <typename T> auto Settings::randomChoice(const T &options, size_t count) {
    std::unordered_set<size_t> indices;
    std::uniform_int_distribution<size_t> iDis(0, options.size() - 1);
    while (indices.size() < count) indices.insert(iDis(rng));
    T chosen;
    chosen.reserve(count);
    for (auto idx : indices) chosen.push_back(options[idx]);
    return chosen;
}

double Settings::random(double min, double max) {
    std::uniform_real_distribution<double> rDis(min, max);
    return rDis(rng);
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

EnumArray<unsigned int, Stat> Settings::travelerStats() {
    // Randomize traveler stats.
    static std::uniform_int_distribution<unsigned int> stDis(1, statMax);
    EnumArray<unsigned int, Stat> stats;
    for (auto &stat : stats) stat = stDis(rng);
    return stats;
}

Part Settings::part() {
    static std::uniform_int_distribution<unsigned int> ptDis(0, static_cast<size_t>(Part::count) - 1);
    return static_cast<Part>(ptDis(rng));
}

AIRole Settings::aIRole() {
    // Randomize ai role.
    static std::discrete_distribution<int> rlDis(begin(aIRoleWeights), end(aIRoleWeights));
    return static_cast<AIRole>(rlDis(rng));
}

std::vector<std::pair<unsigned int, double>> Settings::getAIStartingGoods(AIRole rl) {
    return randomChoice(aIStartingGoods[rl].goods, aIStartingGoods[rl].count);
}

std::vector<unsigned int> Settings::aIFullIds(const std::vector<unsigned int> &fIds) {
    return randomChoice(fIds, aIGoodsCount);
}

EnumArray<double, DecisionCriteria> Settings::aIDecisionCriteria() {
    // Randomize decision criteria.
    static std::uniform_real_distribution<double> dcCrtDis(1, aIDecisionCriteriaMax);
    EnumArray<double, DecisionCriteria> dcCrt;
    for (auto &dC : dcCrt) dC = dcCrtDis(rng);
    return dcCrt;
}

int Settings::aIDecisionCounter() {
    // Randomize decision counter.
    static std::uniform_int_distribution<> dcCntDis(-aIDecisionTime, 0);
    return dcCntDis(rng);
}

int Settings::aIBusinessCounter() {
    // Randomize business counter.
    static std::uniform_int_distribution<> dcCntDis(-aIBusinessInterval, 0);
    return dcCntDis(rng);
}

double Settings::aILimitFactor() {
    static std::uniform_real_distribution<double> lFDis(aILimitFactorMin, aILimitFactorMax);
    return lFDis(rng);
}

BoxInfo Settings::boxInfo(const SDL_Rect &rt, const std::vector<std::string> &tx, ColorScheme sm,
                          std::pair<unsigned int, bool> i, BoxSizeType sz, BoxBehavior bvr, SDL_Keycode ky,
                          const std::function<void(MenuButton *)> &fn, const SDL_Rect &oR) {
    return {rt, tx, sm, i, boxSizes[sz], bvr, {}, ky, fn, oR};
}
