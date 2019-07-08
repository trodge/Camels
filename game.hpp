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

#ifndef GAME_H
#define GAME_H

#include <algorithm>
#include <fstream>
#include <iostream>
#include <map>
#include <string>
#include <vector>

#include <SDL2/SDL_image.h>

#include "business.hpp"
#include "nation.hpp"
#include "player.hpp"
#include "selectbutton.hpp"
#include "town.hpp"
#include "traveler.hpp"

class Player;
class Nation;

class Game {
    SDL_Window *window;
    SDL_Renderer *screen;
    SDL_Surface *mapSurface; // surface of entire map loaded from image
    SDL_RendererInfo screenInfo;
    std::vector<SDL_Texture *> mapTextures;        // textures for map broken down to maximum size for graphics card
    int mapTextureRowCount, mapTextureColumnCount; // number of columns in map textures matrix
    SDL_Texture *mapTexture;                       // texture for drawing map on screen at current position
    SDL_Rect screenRect, mapRect;
    int offsetX, offsetY;
    unsigned int lastTime = 0, currentTime;
    double scale;
    std::vector<Nation> nations;
    std::vector<Town> towns;
    std::vector<Business> businesses;
    GameData gameData;
    std::vector<std::shared_ptr<Traveler>> aITravelers;
    std::unique_ptr<Player> player;
    void loadDisplayVariables();
    void renderMapTexture();
    void loadNations(sqlite3 *c);
    void handleEvents();
    void update();
    void draw();

  public:
    Game();
    ~Game();
    void run();
    void newGame();
    void place();
    void moveView(int dx, int dy);
    void saveGame();
    void loadGame(const fs::path &p);
    void saveData();
    void saveRoutes();
    const SDL_Rect &getMapRect() const { return mapRect; }
    const std::vector<Nation> &getNations() { return nations; }
    const std::vector<Town> &getTowns() { return towns; }
    const GameData &getData() const { return gameData; }
    void unFocusTown(size_t i) { towns[i].unFocus(); }
    void fillFocusableTowns(std::vector<Focusable *> &fcbls);
    std::shared_ptr<Traveler> createPlayerTraveler(size_t nId, std::string n);
    void pickTown(std::shared_ptr<Traveler> t, size_t tId);
};

#endif
