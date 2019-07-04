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

#include "save_generated.h"

#include "selectbutton.h"
#include "town.h"

class Nation;

class Game {
    SDL_Window *window;
    SDL_Renderer *screen;
    bool stop = false, pause = false, storedPause = true;
    SDL_Surface *mapSurface; // surface of entire map loaded from image
    SDL_RendererInfo screenInfo;
    std::vector<SDL_Texture *> mapTextures; // textures for map broken down to maximum size for graphics card
    int mapTextureRowCount, mapTextureColumnCount; // number of columns in map textures matrix
    SDL_Texture *mapTexture; // texture for drawing map on screen at current position
    SDL_Rect screenRect, mapRect;
    int scrollSpeed, scrollX = 0, scrollY = 0;
    int offsetX, offsetY;
    unsigned int lastTime = 0, currentTime;
    double scale;
    std::vector<Nation> nations;
    std::vector<Town> towns;
    int focusTown = -1;
    std::vector<Business> businesses;
    std::vector<std::unique_ptr<TextBox>> boxes, storedBoxes;
    int focusBox = -1;
    enum UIState {
        starting,
        quitting,
        loading,
        traveling,
        logging,
        trading,
        equipping,
        storing,
        managing,
        attacking,
        fighting,
        looting,
        dying
    };
    UIState state, storedState = starting;
    GameData gameData;
    std::vector<std::shared_ptr<Traveler>> travelers;
    Traveler *player = nullptr;
    bool showPlayer = false;
    void loadDisplayVariables();
    void renderMapTexture();
    void changeOffsets(int dx, int dy);
    void loadNations(sqlite3 *c);
    void newGame();
    void load(const fs::path &p);
    void prepFocus(Focusable::FocusGroup g, int &i, int &s, std::vector<Focusable *> &fcbls);
    void finishFocus(int f, Focusable::FocusGroup g, const std::vector<Focusable *> &fcbls);
    void focus(int f, Focusable::FocusGroup g);
    void focusPrev(Focusable::FocusGroup g);
    void focusNext(Focusable::FocusGroup g);
    void createStartButtons();
    void createQuitButtons();
    void createLoadButtons();
    void createTravelButtons();
    void createLogBox();
    void createTradeButtons();
    void createEquipButtons();
    void createStorageButtons();
    void createManageButtons();
    void createAttackButton();
    void createFightBoxes();
    void createLootButtons();
    void createDyingBoxes();
    void makeTrade();
    void setState(UIState s);
    void toggleState(UIState s);
    void updateUI();
    void handleEvents();
    void update();
    void draw();
    void saveRoutes();
    void saveData();
    void save();

  public:
    Game();
    ~Game();
};

#endif
