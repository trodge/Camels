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
std::array<ColorScheme, static_cast<size_t>(ColorSchemeType::count)> Settings::colorSchemes;
SDL_Color Settings::routeColor;
SDL_Color Settings::waterColor;
int Settings::scroll;
SDL_Point Settings::offset;
double Settings::scale;
std::array<BoxSize, static_cast<size_t>(BoxSizeType::count)> Settings::boxSizes;
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
int Settings::attackDistSq;
double Settings::escapeChance;
std::vector<std::pair<unsigned int, double>> Settings::playerStartingGoods;
SDL_Color Settings::playerColor;
std::array<double, static_cast<size_t>(AIRole::count)> Settings::aIRoleWeights;
std::array<std::vector<std::pair<unsigned int, double>>, static_cast<size_t>(AIRole::count)> Settings::aIStartingGoods;
std::array<unsigned int, static_cast<size_t>(AIRole::count)> Settings::aIStartingGoodsCount;
int Settings::aIDecisionCriteriaMax;
unsigned int Settings::aITownRange;
double Settings::aILimitFactorMin, Settings::aILimitFactorMax;
double Settings::aIAttackThreshold;
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

template <typename First, typename Second> std::pair<First, Second> loadPair(const std::string &n, const std::pair<First, Second> &df, const pt::ptree &t) {
    return std::make_pair(t.get(n + "_first", df.first), t.get(n + "_second", df.second));
}

template <class OutputIt, typename T, typename Function>
void loadRange(const std::string &n, OutputIt out, const Function &fn,
               const std::initializer_list<T> &dfs, const pt::ptree &t) {
    unsigned int i = 0;
    std::transform(begin(dfs), end(dfs), out, [&n, &i, &fn, &t](auto &df) { return fn(n + "_" + std::to_string(++i), df, t); });
}

template <class OutputIt, typename T>
void loadRange(const std::string &n, OutputIt out, const std::initializer_list<T> &dfs, const pt::ptree &t) {
    loadRange(n, out, [](const std::string &m, const T &df, const pt::ptree &r) { return r.get(m, df); }, dfs, t);
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
    colorSchemes[static_cast<size_t>(ColorSchemeType::ui)] =
        loadColorScheme("ui.colors", {{0, 0, 0, 255}, {109, 109, 109, 255}, {0, 127, 251, 255}}, tree);
    colorSchemes[static_cast<size_t>(ColorSchemeType::load)] =
        loadColorScheme("ui.loadColors", {{0, 255, 0, 255}, {0, 0, 0, 255}, {0, 0, 0, 0}}, tree);
    routeColor = loadColor("ui.routeColor", {97, 97, 97, 255}, tree);
    waterColor = loadColor("ui.waterColor", {29, 109, 149, 255}, tree);
    scroll = tree.get("ui.scroll", 64);
    offset.x = tree.get("ui.offsetX", -3197);
    offset.y = tree.get("ui.offsetY", 4731);
    scale = tree.get("ui.scale", 112.5);
    boxSizes[static_cast<size_t>(BoxSizeType::big)] =
        loadBoxSize("ui.big", {current.h * 4 / 1080, current.h * 13 / 1080, current.h * 48 / 1080}, tree);
    boxSizes[static_cast<size_t>(BoxSizeType::load)] =
        loadBoxSize("ui.load", {current.h * 4 / 1080, current.h * 13 / 1080, current.h * 36 / 1080}, tree);
    boxSizes[static_cast<size_t>(BoxSizeType::small)] =
        loadBoxSize("ui.small", {current.h * 3 / 1080, current.h * 7 / 1080, current.h * 24 / 1080}, tree);
    boxSizes[static_cast<size_t>(BoxSizeType::town)] = loadBoxSize(
        "ui.town", {std::max(current.h / 1080, 1), std::max(current.h / 1080, 1), current.h * 18 / 1080}, tree);
    boxSizes[static_cast<size_t>(BoxSizeType::trade)] =
        loadBoxSize("ui.trade", {current.h * 2 / 1080, current.h * 5 / 1080, current.h * 16 / 1080}, tree);
    boxSizes[static_cast<size_t>(BoxSizeType::equip)] =
        loadBoxSize("ui.equip", {current.h * 5 / 1080, current.h * 7 / 1080, current.h * 20 / 1080}, tree);
    boxSizes[static_cast<size_t>(BoxSizeType::fight)] =
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
    loadRange("player.startingGoods", std::back_inserter(playerStartingGoods),
              &loadPair<unsigned int, double>, {std::make_pair(55, 0.75), std::make_pair(59, 2.)}, tree);
    playerColor = loadColor("player.color", {255, 255, 255, 255}, tree);
    aIDecisionCriteriaMax = tree.get("aI.decisionCriteriaMax", 9);
    loadRange("aI.roleWeights", begin(aIRoleWeights), {0.4, 0.1, 0.1, 0.2, 0.1, 0.1}, tree);
    loadRange("aI.traderGoods", std::back_inserter(aIStartingGoods[static_cast<size_t>(AIRole::trader)]), &loadPair<unsigned int, double>,
              {std::make_pair(1, 21.), std::make_pair(9, 10.5), std::make_pair(55, 0.75), std::make_pair(59, 2.)}, tree);
    loadRange("aI.soldierGoods", std::back_inserter(aIStartingGoods[static_cast<size_t>(AIRole::soldier)]), &loadPair<unsigned int, double>,
              {std::make_pair(37, 1.), std::make_pair(38, 1.), std::make_pair(42, 1.), std::make_pair(43, 1.),
               std::make_pair(44, 1.), std::make_pair(46, 1.)},
              tree);
    loadRange("aI.banditGoods", std::back_inserter(aIStartingGoods[static_cast<size_t>(AIRole::bandit)]), &loadPair<unsigned int, double>,
              {std::make_pair(37, 1.), std::make_pair(38, 1.), std::make_pair(42, 1.), std::make_pair(43, 1.),
               std::make_pair(44, 1.), std::make_pair(46, 1.)},
              tree);
    loadRange("aI.agentGoods", std::back_inserter(aIStartingGoods[static_cast<size_t>(AIRole::agent)]), &loadPair<unsigned int, double>,
              {std::make_pair(55, 0.75), std::make_pair(59, 2.)}, tree);
    loadRange("aI.guardGoods", std::back_inserter(aIStartingGoods[static_cast<size_t>(AIRole::guard)]), &loadPair<unsigned int, double>,
              {std::make_pair(37, 1.), std::make_pair(38, 1.), std::make_pair(42, 1.), std::make_pair(43, 1.),
               std::make_pair(44, 1.), std::make_pair(46, 1.)},
              tree);
    loadRange("aI.thugGoods", std::back_inserter(aIStartingGoods[static_cast<size_t>(AIRole::thug)]), &loadPair<unsigned int, double>,
              {std::make_pair(37, 1.), std::make_pair(38, 1.), std::make_pair(42, 1.), std::make_pair(43, 1.),
               std::make_pair(44, 1.), std::make_pair(46, 1.)},
              tree);
    aIStartingGoodsCount[static_cast<size_t>(AIRole::trader)] = tree.get("aI.traderGoodCount", 2);
    aIStartingGoodsCount[static_cast<size_t>(AIRole::soldier)] = tree.get("aI.soldierGoodCount", 3);
    aIStartingGoodsCount[static_cast<size_t>(AIRole::bandit)] = tree.get("aI.banditGoodCount", 1);
    aIStartingGoodsCount[static_cast<size_t>(AIRole::agent)] = tree.get("aI.agentGoodCount", 1);
    aIStartingGoodsCount[static_cast<size_t>(AIRole::guard)] = tree.get("aI.guardGoodCount", 2);
    aIStartingGoodsCount[static_cast<size_t>(AIRole::thug)] = tree.get("aI.thugGoodCount", 1);
    aITownRange = static_cast<unsigned int>(tree.get("aI.townRange", 9));
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

template <typename First, typename Second> void savePair(const std::string &n, const std::pair<First, Second> &pr, pt::ptree &t) {
    t.put(n + "_first", pr.first);
    t.put(n + "_second", pr.second);
}

template <class InputIt, typename Function> void saveRange(const std::string &n, InputIt bgn, InputIt end,
                                                    const Function &fn, pt::ptree &t) {
    unsigned int i = 0;
    std::for_each(bgn, end, [&n, &i, &fn, &t](auto &a) { fn(n + "_" + std::to_string(++i), a, t); });
}

template <class InputIt> void saveRange(const std::string &n, InputIt bgn, InputIt end, pt::ptree &t) {
    saveRange(n, bgn, end, [](const std::string &m, const decltype(*bgn) &sv, pt::ptree &r) { r.put(m, sv); }, t);
}

void Settings::save(const fs::path &p) {
    pt::ptree tree;
    saveRect("ui.mapView", mapView, tree);
    saveColorScheme("ui.colors", colorSchemes[static_cast<size_t>(ColorSchemeType::ui)], tree);
    saveColorScheme("ui.loadColors", colorSchemes[static_cast<size_t>(ColorSchemeType::load)], tree);
    saveColor("ui.routeColor", routeColor, tree);
    saveColor("ui.waterColor", waterColor, tree);
    tree.put("ui.scroll", scroll);
    tree.put("ui.offsetX", offset.x);
    tree.put("ui.offsetY", offset.y);
    tree.put("ui.scale", scale);
    saveBoxSize("ui.big", boxSizes[static_cast<size_t>(BoxSizeType::big)], tree);
    saveBoxSize("ui.load", boxSizes[static_cast<size_t>(BoxSizeType::load)], tree);
    saveBoxSize("ui.small", boxSizes[static_cast<size_t>(BoxSizeType::small)], tree);
    saveBoxSize("ui.town", boxSizes[static_cast<size_t>(BoxSizeType::town)], tree);
    saveBoxSize("ui.trade", boxSizes[static_cast<size_t>(BoxSizeType::trade)], tree);
    saveBoxSize("ui.equip", boxSizes[static_cast<size_t>(BoxSizeType::equip)], tree);
    saveBoxSize("ui.fight", boxSizes[static_cast<size_t>(BoxSizeType::fight)], tree);
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
    saveRange("player.startingGoods", begin(playerStartingGoods),
              end(playerStartingGoods), &savePair<unsigned int, double>, tree);
    saveColor("player.color", playerColor, tree);
    tree.put("aI.decisionCriteriaMax", aIDecisionCriteriaMax);
    saveRange("aI.roleWeights", begin(aIRoleWeights), end(aIRoleWeights), tree);
    saveRange("aI.traderGoods", begin(aIStartingGoods[static_cast<size_t>(AIRole::trader)]),
              end(aIStartingGoods[static_cast<size_t>(AIRole::trader)]), &savePair<unsigned int, double>, tree);
    saveRange("aI.soldierGoods", begin(aIStartingGoods[static_cast<size_t>(AIRole::soldier)]),
              end(aIStartingGoods[static_cast<size_t>(AIRole::soldier)]), &savePair<unsigned int, double>, tree);
    saveRange("aI.banditGoods", begin(aIStartingGoods[static_cast<size_t>(AIRole::bandit)]),
              end(aIStartingGoods[static_cast<size_t>(AIRole::bandit)]), &savePair<unsigned int, double>, tree);
    saveRange("aI.agentGoods", begin(aIStartingGoods[static_cast<size_t>(AIRole::agent)]),
              end(aIStartingGoods[static_cast<size_t>(AIRole::agent)]), &savePair<unsigned int, double>, tree);
    saveRange("aI.guardGoods", begin(aIStartingGoods[static_cast<size_t>(AIRole::guard)]),
              end(aIStartingGoods[static_cast<size_t>(AIRole::guard)]), &savePair<unsigned int, double>, tree);
    saveRange("aI.thugGoods", begin(aIStartingGoods[static_cast<size_t>(AIRole::thug)]),
              end(aIStartingGoods[static_cast<size_t>(AIRole::thug)]), &savePair<unsigned int, double>, tree);
    tree.put("aI.traderGoodCount", aIStartingGoodsCount[static_cast<size_t>(AIRole::trader)]);
    tree.put("aI.soldierGoodCount", aIStartingGoodsCount[static_cast<size_t>(AIRole::soldier)]);
    tree.put("aI.banditGoodCount", aIStartingGoodsCount[static_cast<size_t>(AIRole::bandit)]);
    tree.put("aI.agentGoodCount", aIStartingGoodsCount[static_cast<size_t>(AIRole::agent)]);
    tree.put("aI.guardGoodCount", aIStartingGoodsCount[static_cast<size_t>(AIRole::guard)]);
    tree.put("aI.thugGoodCount", aIStartingGoodsCount[static_cast<size_t>(AIRole::thug)]);
    tree.put("aI.townRange", aITownRange);
    tree.put("aI.limitFactorMin", aILimitFactorMin );
    tree.put("aI.limitFactorMax", aILimitFactorMax );
    tree.put("aI.attackThreshold", aIAttackThreshold);
    saveColor("ui.aIColor", aIColor, tree);
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

std::array<unsigned int, static_cast<size_t>(Stat::count)> Settings::travelerStats() {
    // Randomize traveler stats.
    static std::uniform_int_distribution<unsigned int> stDis(1, statMax);
    std::array<unsigned int, static_cast<size_t>(Stat::count)> stats;
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
    auto &startingGoods = aIStartingGoods[static_cast<size_t>(rl)];
    std::unordered_set<size_t> indices;
    std::uniform_int_distribution<size_t> iDis(0, startingGoods.size() - 1);
    auto goodCount = aIStartingGoodsCount[static_cast<size_t>(rl)];
    while (indices.size() < goodCount) indices.insert(iDis(rng));
    std::vector<std::pair<unsigned int, double>> chosenGoods;
    chosenGoods.reserve(goodCount);
    for (auto idx : indices) chosenGoods.push_back(startingGoods[idx]);
    return chosenGoods;
}

std::array<double, static_cast<size_t>(DecisionCriteria::count)> Settings::aIDecisionCriteria() {
    // Randomize decision criteria.
    static std::uniform_real_distribution<double> dcCrtDis(1, aIDecisionCriteriaMax);
    std::array<double, static_cast<size_t>(DecisionCriteria::count)> dcCrt;
    for (auto &dC : dcCrt) dC = dcCrtDis(rng);
    return dcCrt;
}

double Settings::aIDecisionCounter() {
    // Randomize decision counter.
    static std::uniform_int_distribution<> dcCntDis(-aIDecisionTime, 0);
    return dcCntDis(rng);
}

double Settings::aILimitFactor() {
    static std::uniform_real_distribution<double> lFDis(aILimitFactorMin, aILimitFactorMax);
    return lFDis(rng);
}

BoxInfo Settings::boxInfo(const SDL_Rect &rt, const std::vector<std::string> &tx, ColorScheme sm,
                          std::pair<unsigned int, bool> i, BoxSizeType sz, BoxBehavior bvr, SDL_Keycode ky,
                          const std::function<void(MenuButton *)> &fn, const SDL_Rect &oR) {
    return {rt, tx, sm, i, boxSizes[static_cast<std::size_t>(sz)], bvr, {}, ky, fn, oR};
}
