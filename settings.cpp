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

#include "settings.h"

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
int Settings::bigBoxBorder;
int Settings::bigBoxRadius;
int Settings::bigBoxFontDivisor;
int Settings::loadBarFontDivisor;
int Settings::smallBoxBorder;
int Settings::smallBoxRadius;
int Settings::smallBoxFontDivisor;
int Settings::fightFontDivisor;
int Settings::equipFontDivisor;
int Settings::townFontDivisor;
int Settings::tradeFontDivisor;
int Settings::goodButtonXDivisor, Settings::goodButtonYDivisor;
double Settings::scale;
int Settings::dayLength;
int Settings::businessRunTime;
double Settings::consumptionSpaceFactor, Settings::inputSpaceFactor, Settings::outputSpaceFactor;
double Settings::townProfit;
int Settings::minPriceDivisor;
double Settings::travelersExponent;
int Settings::travelersMin;
int Settings::statMax;
double Settings::attackDistSq;
double Settings::escapeChance;
int Settings::aIDecisionTime;
int Settings::criteriaMax;
unsigned int Settings::aITownRange;
double Settings::limitFactorMin, Settings::limitFactorMax;
double Settings::aIAttackThreshold;
std::mt19937 Settings::rng(static_cast<unsigned long>(std::chrono::high_resolution_clock::now().time_since_epoch().count()));

void loadRect(const std::string &n, SDL_Rect *r, const SDL_Rect &d, const pt::ptree &t) {
    r->x = t.get(n + "_x", d.x);
    r->y = t.get(n + "_y", d.y);
    r->w = t.get(n + "_w", d.w);
    r->h = t.get(n + "_h", d.h);
}

void loadColor(const std::string &n, SDL_Color *c, const SDL_Color &d, const pt::ptree &t) {
    c->r = t.get(n + "_r", d.r);
    c->g = t.get(n + "_g", d.g);
    c->b = t.get(n + "_b", d.b);
}

void Settings::load(const fs::path &p) {
    pt::ptree tree;
    pt::read_ini(p.string(), tree);
    SDL_DisplayMode current;
    SDL_GetCurrentDisplayMode(0, &current);
    loadRect("ui.mapRect", &mapRect, {3330, 2250, current.w * 14 / 15, current.h * 14 / 15}, tree);
    loadColor("ui.foreground", &uIForeground, {0, 0, 0, 255}, tree);
    loadColor("ui.background", &uIBackground, {109, 109, 109, 255}, tree);
    loadColor("ui.highlight", &uIHighlight, {0, 127, 251, 255}, tree);
    loadColor("ui.loadBarColor", &loadBarColor, {0, 255, 0, 255}, tree);
    loadColor("ui.routeColor", &routeColor, {97, 97, 97, 255}, tree);
    loadColor("ui.waterColor", &waterColor, {29, 109, 149, 255}, tree);
    loadColor("ui.playerColor", &playerColor, {255, 255, 255, 255}, tree);
    loadColor("ui.aIColor", &aIColor, {191, 191, 191, 255}, tree);
    scroll = tree.get("ui.scroll", 64);
    offsetX = tree.get("ui.offsetX", -3197);
    offsetY = tree.get("ui.offsetY", 4725);
    bigBoxBorder = tree.get("ui.bigBoxBorder", 4);
    bigBoxRadius = tree.get("ui.bigBoxRadius", 7);
    bigBoxFontDivisor = tree.get("ui.bigBoxFontDivisor", 25);
    loadBarFontDivisor = tree.get("ui.loadBarFontDivisor", 26);
    smallBoxBorder = tree.get("ui.smallBoxBorder", 3);
    smallBoxRadius = tree.get("ui.smallBoxRadius", 5);
    smallBoxFontDivisor = tree.get("ui.smallBoxFontDivisor", 54);
    fightFontDivisor = tree.get("ui.fightFontDivisor", 49);
    equipFontDivisor = tree.get("ui.equipFontDivisor", 60);
    townFontDivisor = tree.get("ui.townFontDivisor", 77);
    tradeFontDivisor = tree.get("ui.tradeFontDivisor", 77);
    goodButtonXDivisor = tree.get("ui.goodButtonXDivisor", 434);
    goodButtonYDivisor = tree.get("ui.goodButtonYDivisor", 527);
    scale = tree.get("ui.scale", 112.5);
    dayLength = tree.get("time.dayLength", 5000);
    businessRunTime = tree.get("time.businessRunTime", 1500);
    consumptionSpaceFactor = tree.get("goods.consumptionSpaceFactor", 0.036);
    inputSpaceFactor = tree.get("goods.inputSpaceFactor", 0.054);
    outputSpaceFactor = tree.get("goods.outputSpaceFactor", 0.054);
    townProfit = tree.get("goods.townProfit", 0.9);
    minPriceDivisor = tree.get("goods.minPriceDivisor", 2047);
    travelersExponent = tree.get("travelers.exponent", 0.24);
    travelersMin = tree.get("travelers.min", -13);
    statMax = tree.get("travelers.statMax", 15);
    attackDistSq = tree.get("travelers.attackDistSq", 9000);
    escapeChance = tree.get("travelers.escapeChance", 0.5);
    aIDecisionTime = tree.get("ai.decisionTime", 12000);
    criteriaMax = tree.get("ai.criteriaMax", 9);
    aITownRange = tree.get("ai.townRange", 9);
    limitFactorMin = tree.get("ai.limitFactorMin", 0.1);
    limitFactorMax = tree.get("ai.limitFactorMax", 0.9);
    aIAttackThreshold = tree.get("ai.attackThreshold", 13500);
}

void saveRect(const std::string &n, const SDL_Rect &r, pt::ptree *t) {
    t->put(n + "_x", r.x);
    t->put(n + "_y", r.y);
    t->put(n + "_w", r.w);
    t->put(n + "_h", r.h);
}

void saveColor(const std::string &n, const SDL_Color &c, pt::ptree *t) {
    t->put(n + "_r", c.r);
    t->put(n + "_g", c.g);
    t->put(n + "_b", c.b);
}

void Settings::save(const fs::path &p) {
    pt::ptree tree;
    saveRect("ui.mapRect", mapRect, &tree);
    saveColor("ui.foreground", uIForeground, &tree);
    saveColor("ui.background", uIBackground, &tree);
    saveColor("ui.highlight", uIHighlight, &tree);
    saveColor("ui.routeColor", routeColor, &tree);
    saveColor("ui.waterColor", waterColor, &tree);
    saveColor("ui.playerColor", playerColor, &tree);
    saveColor("ui.aIColor", aIColor, &tree);
    tree.put("ui.scroll", scroll);
    tree.put("ui.offsetX", offsetX);
    tree.put("ui.offsetY", offsetY);
    tree.put("ui.bigBoxFontDivisor", bigBoxFontDivisor);
    tree.put("ui.bigBoxBorder", bigBoxBorder);
    tree.put("ui.loadBarFontDivisor", loadBarFontDivisor);
    tree.put("ui.smallBoxFontDivisor", smallBoxFontDivisor);
    tree.put("ui.smallBoxBorder", smallBoxBorder);
    tree.put("ui.fightFontDivisor", fightFontDivisor);
    tree.put("ui.equipFontDivisor", equipFontDivisor);
    tree.put("ui.townFontDivisor", townFontDivisor);
    tree.put("ui.tradeFontDivisor", tradeFontDivisor);
    tree.put("ui.goodButtonXDivisor", goodButtonXDivisor);
    tree.put("ui.goodButtonYDivisor", goodButtonYDivisor);
    tree.put("time.dayLength", dayLength);
    tree.put("time.businessRunTime", businessRunTime);
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
    tree.put("ai.decisionTime", aIDecisionTime);
    tree.put("ai.criteriaMax", criteriaMax);
    tree.put("ai.townRange", aITownRange);
    tree.put("ai.limitFactorMin", limitFactorMin);
    tree.put("ai.limitFactorMax", limitFactorMax);
    tree.put("ai.attackThreshold", aIAttackThreshold);
    pt::write_ini(p.string(), tree);
}
