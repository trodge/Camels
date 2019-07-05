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

#include "player.hpp"
#include "traveler.hpp"
#include "selectbutton.hpp"
#include "nation.hpp"
#include "town.hpp"
#include "business.hpp"

class Player;
class Nation;

class Game {
    SDL_Window *window;
    SDL_Renderer *screen;
    bool stop = false, pause = false, storedPause = true;
    SDL_Surface *mapSurface; // surface of entire map loaded from image
    SDL_RendererInfo screenInfo;
    std::vector<SDL_Texture *> mapTextures;        // textures for map broken down to maximum size for graphics card
    int mapTextureRowCount, mapTextureColumnCount; // number of columns in map textures matrix
    SDL_Texture *mapTexture;                       // texture for drawing map on screen at current position
    SDL_Rect screenRect, mapRect;
    int scrollSpeed, scrollX = 0, scrollY = 0;
    bool showPlayer = false;
    int offsetX, offsetY;
    unsigned int lastTime = 0, currentTime;
    double scale;
    std::vector<Nation> nations;
    std::vector<Town> towns;
    std::vector<Business> businesses;
    GameData gameData;
    std::vector<std::shared_ptr<Traveler>> travelers;
    Player player;
    void loadDisplayVariables();
    void renderMapTexture();
    void moveView(int dx, int dy);
    void loadNations(sqlite3 *c);
    void handleEvents();
    void update();
    void draw();

  public:
    Game();
    ~Game();
    void newGame();
    void place();
    void saveGame();
    void loadGame(const fs::path &p);
    void saveData();
    void saveRoutes();
};

#endif
