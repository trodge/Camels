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

#include <boost/filesystem.hpp>
#include <boost/property_tree/ini_parser.hpp>
#include <boost/property_tree/ptree.hpp>
#include <chrono>
#include <random>
namespace fs = boost::filesystem;
namespace pt = boost::property_tree;

#include <SDL2/SDL.h>

class Settings {
    static SDL_Rect mapRect;
    static SDL_Color uIForeground;
    static SDL_Color uIBackground;
    static SDL_Color uIHighlight;
    static SDL_Color routeColor;
    static SDL_Color dotColor;
    static SDL_Color waterColor;
    static SDL_Color playerColor;
    static SDL_Color aIColor;
    static int scroll;
    static int offsetX, offsetY;
    static int bigBoxFontSize;
    static int bigBoxBorder;
    static int loadBarFontSize;
    static int smallBoxFontSize;
    static int smallBoxBorder;
    static int fightFontSize;
    static int equipFontSize;
    static int townFontSize;
    static int tradeFontSize;
    static int goodButtonXDivisor, goodButtonYDivisor;
    static double scale;
    static int dayLength;
    static int businessRunTime;
    static double consumptionSpaceFactor, inputSpaceFactor, outputSpaceFactor;
    static double townProfit;
    static int minPriceDivisor;
    static double travelersExponent;
    static int travelersMin;
    static int statMax;
    static double attackDistSq;
    static double escapeChance;
    static int aIDecisionTime;
    static int criteriaMax;
    static int aITownRange;
    static double limitFactorMin, limitFactorMax;
    static double aIAttackThreshold;
    static std::mt19937 rng;

  public:
    static void load(const fs::path &p);
    static void save(const fs::path &p);
    static SDL_Rect getMapRect() { return mapRect; }
    static SDL_Color getUIForeground() { return uIForeground; }
    static SDL_Color getUIBackground() { return uIBackground; }
    static SDL_Color getUIHighlight() { return uIHighlight; }
    static SDL_Color getRouteColor() { return routeColor; }
    static SDL_Color getDotColor() { return dotColor; }
    static SDL_Color getWaterColor() { return waterColor; }
    static SDL_Color getPlayerColor() { return playerColor; }
    static SDL_Color getAIColor() { return aIColor; }
    static int getScroll() { return scroll; }
    static int getOffsetX() { return offsetX; }
    static int getOffsetY() { return offsetY; }
    static int getBigBoxFontSize() { return bigBoxFontSize; }
    static int getBigBoxBorder() { return bigBoxBorder; }
    static int getLoadBarFontSize() { return loadBarFontSize; }
    static int getSmallBoxFontSize() { return smallBoxFontSize; }
    static int getSmallBoxBorder() { return smallBoxBorder; }
    static int getFightFontSize() { return fightFontSize; }
    static int getEquipFontSize() { return equipFontSize; }
    static int getTownFontSize() { return townFontSize; }
    static int getTradeFontSize() { return tradeFontSize; }
    static int getGoodButtonXDivisor() { return goodButtonXDivisor; }
    static int getGoodButtonYDivisor() { return goodButtonYDivisor; }
    static double getScale() { return scale; }
    static int getDayLength() { return dayLength; }
    static int getBusinessRunTime() { return businessRunTime; }
    static double getConsumptionSpaceFactor() { return consumptionSpaceFactor; }
    static double getInputSpaceFactor() { return inputSpaceFactor; }
    static double getOutputSpaceFactor() { return outputSpaceFactor; }
    static double getTownProfit() { return townProfit; }
    static int getMinPriceDivisor() { return minPriceDivisor; }
    static double getTravelersExponent() { return travelersExponent; }
    static int getTravelersMin() { return travelersMin; }
    static int getStatMax() { return statMax; }
    static double getAttackDistSq() { return attackDistSq; }
    static double getEscapeChance() { return escapeChance; }
    static int getAIDecisionTime() { return aIDecisionTime; }
    static int getCriteriaMax() { return criteriaMax; }
    static int getAITownRange() { return aITownRange; }
    static double getLimitFactorMin() { return limitFactorMin; }
    static double getLimitFactorMax() { return limitFactorMax; }
    static double getAIAttackThreshold() { return aIAttackThreshold; }
    static std::mt19937 *getRng() { return &rng; }
};

#endif // SETTINGS_H