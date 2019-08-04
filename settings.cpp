/*
 * This file is part of Camels.
 *
 * Camels is free software: you can redistribute it and/or modify
 * it under the terms of the gNU general Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Camels is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * gNU general Public License for more details.
 *
 * You should have received a copy of the gNU general Public License
 * along with Camels.  If not, see <https://www.gnu.org/licenses/>.
 *
 * © Tom Rodgers notaraptor@gmail.com 2017-2019
 */

#include "settings.hpp"

SDL_Rect Settings::screenRect;
SDL_Rect Settings::mapRect;
SDL_Color Settings::uIForeground;
SDL_Color Settings::uIBackground;
SDL_Color Settings::uIHighlight;
SDL_Color Settings::loadBarColor;
SDL_Color Settings::routeColor;
SDL_Color Settings::waterColor;
SDL_Color Settings::playerColor;
SDL_Color Settings::aIColor;
int Settings::scroll;
int Settings::offsetX;
int Settings::offsetY;
double Settings::scale;
int Settings::bigBoxBorder;
int Settings::bigBoxRadius;
int Settings::bigBoxFontSize;
int Settings::loadBarFontSize;
int Settings::smallBoxBorder;
int Settings::smallBoxRadius;
int Settings::smallBoxFontSize;
int Settings::fightFontSize;
int Settings::equipBorder;
int Settings::equipRadius;
int Settings::equipFontSize;
int Settings::townFontSize;
int Settings::tradeBorder;
int Settings::tradeRadius;
int Settings::tradeFontSize;
int Settings::goodButtonSizeMultiplier;
int Settings::goodButtonSpaceMultiplier;
int Settings::goodButtonXDivisor;
int Settings::goodButtonYDivisor;
int Settings::businessButtonSizeMultiplier;
int Settings::businessButtonSpaceMultiplier;
int Settings::businessButtonXDivisor;
int Settings::businessButtonYDivisor;
int Settings::dayLength;
unsigned int Settings::businessHeadStart;
int Settings::businessRunTime;
int Settings::travelersCheckTime;
int Settings::aIDecisionTime;
double Settings::consumptionSpaceFactor, Settings::inputSpaceFactor, Settings::outputSpaceFactor;
double Settings::townProfit;
int Settings::minPriceDivisor;
double Settings::travelersExponent;
int Settings::travelersMin;
unsigned int Settings::statMax;
double Settings::attackDistSq;
double Settings::escapeChance;
std::array<double, 6> Settings::aITypeWeights;
int Settings::criteriaMax;
unsigned int Settings::aITownRange;
double Settings::limitFactorMin, Settings::limitFactorMax;
double Settings::aIAttackThreshold;

std::mt19937 Settings::rng(static_cast<unsigned long>(std::chrono::high_resolution_clock::now().time_since_epoch().count()));

void loadRect(const std::string &n, SDL_Rect &r, const SDL_Rect &d, const pt::ptree &t) {
    r.x = t.get(n + "_x", d.x);
    r.y = t.get(n + "_y", d.y);
    r.w = t.get(n + "_w", d.w);
    r.h = t.get(n + "_h", d.h);
}

void loadColor(const std::string &n, SDL_Color &c, const SDL_Color &d, const pt::ptree &t) {
    c.r = t.get(n + "_r", d.r);
    c.g = t.get(n + "_g", d.g);
    c.b = t.get(n + "_b", d.b);
    c.a = t.get(n + "_a", d.a);
}

template <class OutputIt, class T>
void loadRange(const std::string &n, OutputIt bgn, const std::initializer_list<T> &dfs, const ::pt::ptree &t) {
    auto it = bgn;
    for (auto &df : dfs) *it++ = t.get(n + "_" + std::to_string(it - bgn), df);
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
    loadRect("ui.screenRect", screenRect, {0, 0, current.w * 14 / 15, current.h * 14 / 15}, tree);
    loadRect("ui.mapRect", mapRect, {3330, 2250, screenRect.w, screenRect.h}, tree);
    loadColor("ui.foreground", uIForeground, {0, 0, 0, 255}, tree);
    loadColor("ui.background", uIBackground, {109, 109, 109, 255}, tree);
    loadColor("ui.highlight", uIHighlight, {0, 127, 251, 255}, tree);
    loadColor("ui.loadBarColor", loadBarColor, {0, 255, 0, 255}, tree);
    loadColor("ui.routeColor", routeColor, {97, 97, 97, 255}, tree);
    loadColor("ui.waterColor", waterColor, {29, 109, 149, 255}, tree);
    loadColor("ui.playerColor", playerColor, {255, 255, 255, 255}, tree);
    loadColor("ui.aIColor", aIColor, {191, 191, 191, 255}, tree);
    scroll = tree.get("ui.scroll", 64);
    offsetX = tree.get("ui.offsetX", -3197);
    offsetY = tree.get("ui.offsetY", 4731);
    scale = tree.get("ui.scale", 112.5);
    bigBoxBorder = tree.get("ui.bigBoxBorder", current.h * 4 / 1080);
    bigBoxRadius = tree.get("ui.bigBoxRadius", current.h * 13 / 1080);
    bigBoxFontSize = tree.get("ui.bigBoxFontSize", current.h * 48 / 1080);
    loadBarFontSize = tree.get("ui.loadBarFontSize", current.h * 36 / 1080);
    smallBoxBorder = tree.get("ui.smallBoxBorder", current.h * 3 / 1080);
    smallBoxRadius = tree.get("ui.smallBoxRadius", current.h * 7 / 1080);
    smallBoxFontSize = tree.get("ui.smallBoxFontSize", current.h * 24 / 1080);
    fightFontSize = tree.get("ui.fightFontSize", current.h * 20 / 1080);
    equipBorder = tree.get("ui.equipBorder", current.h * 5 / 1080);
    equipRadius = tree.get("ui.equipRadius", current.h * 7 / 1080);
    equipFontSize = tree.get("ui.equipFontSize", current.h * 20 / 1080);
    townFontSize = tree.get("ui.townFontSize", current.h * 18 / 1080);
    tradeBorder = tree.get("ui.tradeBorder", current.h * 2 / 1080);
    tradeRadius = tree.get("ui.tradeRadius", current.h * 5 / 1080);
    tradeFontSize = tree.get("ui.tradeFontSize", current.h * 16 / 1080);
    goodButtonSizeMultiplier = tree.get("ui.goodButtonSizeMultiplier", 29);
    goodButtonSpaceMultiplier = tree.get("ui.goodButtonSpaceMultiplier", 31);
    goodButtonXDivisor = tree.get("ui.goodButtonXDivisor", 155);
    goodButtonYDivisor = tree.get("ui.goodButtonYDivisor", 403);
    businessButtonSizeMultiplier = tree.get("ui.businessButtonSizeMultiplier", 29);
    businessButtonSpaceMultiplier = tree.get("ui.businessButtonSpaceMultiplier", 31);
    businessButtonXDivisor = tree.get("ui.businessButtonXDivisor", 124);
    businessButtonYDivisor = tree.get("ui.businessButtonYDivisor", 93);
    dayLength = tree.get("time.dayLength", 5000);
    businessHeadStart = static_cast<unsigned int>(tree.get("time.businessHeadStart", 15000));
    businessRunTime = tree.get("time.businessRunTime", 1500);
    travelersCheckTime = tree.get("time.travelersCheckTime", 4500);
    aIDecisionTime = tree.get("time.aIDecisionTime", 12000);
    consumptionSpaceFactor = tree.get("goods.consumptionSpaceFactor", 0.036);
    inputSpaceFactor = tree.get("goods.inputSpaceFactor", 0.054);
    outputSpaceFactor = tree.get("goods.outputSpaceFactor", 0.054);
    townProfit = tree.get("goods.townProfit", 0.9);
    minPriceDivisor = tree.get("goods.minPriceDivisor", 2047);
    travelersExponent = tree.get("travelers.exponent", 0.24);
    travelersMin = tree.get("travelers.min", -13);
    statMax = static_cast<unsigned int>(tree.get("travelers.statMax", 15));
    attackDistSq = tree.get("travelers.attackDistSq", 9000);
    escapeChance = tree.get("travelers.escapeChance", 0.5);
    criteriaMax = tree.get("aI.criteriaMax", 9);
    loadRange("aI.typeWeights", begin(aITypeWeights), {0.4, 0.1, 0.1, 0.2, 0.1, 0.1}, tree);
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

template <class InputIt> void saveRange(const std::string &n, InputIt bgn, InputIt end, pt::ptree &t) {
    for (auto it = bgn; it != end; ++it) t.put(n + "_" + std::to_string(it - bgn), *it);
}

void Settings::save(const fs::path &p) {
    pt::ptree tree;
    saveRect("ui.mapRect", mapRect, tree);
    saveColor("ui.foreground", uIForeground, tree);
    saveColor("ui.background", uIBackground, tree);
    saveColor("ui.highlight", uIHighlight, tree);
    saveColor("ui.routeColor", routeColor, tree);
    saveColor("ui.waterColor", waterColor, tree);
    saveColor("ui.playerColor", playerColor, tree);
    saveColor("ui.aIColor", aIColor, tree);
    tree.put("ui.scroll", scroll);
    tree.put("ui.offsetX", offsetX);
    tree.put("ui.offsetY", offsetY);
    tree.put("ui.scale", scale);
    tree.put("ui.bigBoxFontSize", bigBoxFontSize);
    tree.put("ui.bigBoxBorder", bigBoxBorder);
    tree.put("ui.loadBarFontSize", loadBarFontSize);
    tree.put("ui.smallBoxFontSize", smallBoxFontSize);
    tree.put("ui.smallBoxBorder", smallBoxBorder);
    tree.put("ui.fightFontSize", fightFontSize);
    tree.put("ui.equipFontSize", equipFontSize);
    tree.put("ui.townFontSize", townFontSize);
    tree.put("ui.tradeFontSize", tradeFontSize);
    tree.put("time.dayLength", dayLength);
    tree.put("time.businessHeadStart", businessHeadStart);
    tree.put("time.businessRunTime", businessRunTime);
    tree.put("time.travelersCheckTime", travelersCheckTime);
    tree.put("time.aIDecisionTime", aIDecisionTime);
    tree.put("goods.consumptionSpaceFactor", consumptionSpaceFactor);
    tree.put("goods.inputSpaceFactor", inputSpaceFactor);
    tree.put("goods.outputSpaceFactor", outputSpaceFactor);
    tree.put("goods.townProfit", townProfit);
    tree.put("goods.minPriceDivisor", minPriceDivisor);
    tree.put("travelers.exponent", travelersExponent);
    tree.put("travelers.min", travelersMin);
    tree.put("travelers.statMax", statMax);
    tree.put("travelers.attackDistSq", attackDistSq);
    tree.put("travelers.escapeChance", escapeChance);
    tree.put("aI.criteriaMax", criteriaMax);
    saveRange("aI.typeWeights", begin(aITypeWeights), end(aITypeWeights), tree);
    tree.put("aI.townRange", aITownRange);
    tree.put("aI.limitFactorMin", limitFactorMin);
    tree.put("aI.limitFactorMax", limitFactorMax);
    tree.put("aI.attackThreshold", aIAttackThreshold);
    pt::write_ini(p.string(), tree);
}

BoxInfo Settings::getBoxInfo(bool iBg, const SDL_Rect &rt, const std::vector<std::string> &tx, SDL_Keycode ky,
                             std::function<void(MenuButton *)> fn, bool scl) {
    if (iBg)
        return {rt,           tx,           uIForeground,   uIBackground, uIHighlight, 0,  false,
                bigBoxBorder, bigBoxRadius, bigBoxFontSize, {},           ky,          fn, true};
    else
        return {
            rt, tx, uIForeground, uIBackground, uIHighlight, 0, false, smallBoxBorder, smallBoxRadius, smallBoxFontSize,
            {}, ky, fn,           true};
}

BoxInfo Settings::getBoxInfo(bool iBg, const SDL_Rect &rt, const std::vector<std::string> &tx, SDL_Keycode ky,
                             std::function<void(MenuButton *)> fn) {
    if (iBg)
        return {rt,           tx,           uIForeground,   uIBackground, uIHighlight, 0, false,
                bigBoxBorder, bigBoxRadius, bigBoxFontSize, {},           ky,          fn};
    else
        return {rt,
                tx,
                uIForeground,
                uIBackground,
                uIHighlight,
                0,
                false,
                smallBoxBorder,
                smallBoxRadius,
                smallBoxFontSize,
                {},
                ky,
                fn};
}

BoxInfo Settings::getBoxInfo(bool iBg, const SDL_Rect &rt, const std::vector<std::string> &tx) {
    if (iBg)
        return {rt, tx, uIForeground, uIBackground, uIHighlight, 0, false, bigBoxBorder, bigBoxRadius, bigBoxFontSize};
    else
        return {rt, tx,    uIForeground,   uIBackground,   uIHighlight,
                0,  false, smallBoxBorder, smallBoxRadius, smallBoxFontSize};
}
