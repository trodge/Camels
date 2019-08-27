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
#include <sqlite3.h>

#include "business.hpp"
#include "loadbar.hpp"
#include "nation.hpp"
#include "player.hpp"
#include "textbox.hpp"
#include "town.hpp"
#include "traveler.hpp"

namespace sql {
struct Deleter {
    void operator()(sqlite3 *cn) {
        if (sqlite3_close(cn) != SQLITE_OK) std::cout << sqlite3_errmsg(cn) << std::endl;
    }
    void operator()(sqlite3_stmt *qr) {
        if (sqlite3_finalize(qr) != SQLITE_OK)
            std::cout << sqlite3_errmsg(sqlite3_db_handle(qr)) << std::endl;
    }
};

using DtbsPtr = std::unique_ptr<sqlite3, Deleter>;
using StmtPtr = std::unique_ptr<sqlite3_stmt, Deleter>;

inline DtbsPtr makeConnection(const fs::path &path, int flags) {
    sqlite3 *conn;
    if (sqlite3_open_v2(path.string().c_str(), &conn, flags, nullptr) != SQLITE_OK)
        throw std::system_error(sqlite3_errcode(conn), std::generic_category());
    return std::unique_ptr<sqlite3, Deleter>(conn);
}

inline StmtPtr makeQuery(sqlite3 *db, const char *zSql) {
    sqlite3_stmt *quer;
    if (sqlite3_prepare_v2(db, zSql, -1, &quer, nullptr) != SQLITE_OK)
        throw std::system_error(sqlite3_errcode(db), std::generic_category());
    return std::unique_ptr<sqlite3_stmt, Deleter>(quer);
}
} // namespace sql

class Player;
class Nation;

class Game {
    SDL_Rect screenRect, mapView, mapRect;
    SDL_Point offset;
    double scale;
    sdl::Handle<SDL_Window> window;
    sdl::Handle<SDL_Renderer> screen;
    // sdl::SurfacePtr mapSurface; // surface of entire map loaded from image
    SDL_RendererInfo screenInfo;
    Printer printer;
    std::vector<sdl::Handle<SDL_Texture>> mapTextures; // textures for map broken down to maximum size for graphics card
    int mapTextureRowCount, mapTextureColumnCount; // number of columns in map textures matrix
    sdl::Texture mapTexture;                       // texture for drawing map on screen at current position
    unsigned int lastTime = 0, currentTime;
    std::vector<Nation> nations;
    std::vector<Town> towns;
    std::vector<Route> routes;
    std::vector<sdl::Surface> goodImages;
    GameData gameData;
    std::vector<std::unique_ptr<Traveler>> aITravelers;
    int travelersCheckCounter;
    std::unique_ptr<Player> player;
    void loadData(sqlite3 *cn);
    void loadTowns(sqlite3 *cn, LoadBar &ldBr, SDL_Texture *frzTx);
    void renderMapTexture();
    void handleEvents();
    void update();
    void draw();

public:
    Game();
    ~Game();
    void run();
    const std::vector<Nation> &newGame();
    void place();
    void moveView(const SDL_Point &dp);
    void saveGame();
    void loadGame(const fs::path &p);
    void saveData();
    void generateRoutes();
    const SDL_Rect &getMapView() const { return mapView; }
    const std::vector<Town> &getTowns() { return towns; }
    std::vector<TextBox *> getTownBoxes() const;
    const GameData &getData() const { return gameData; }
    Printer &getPrinter() { return printer; }
    std::unique_ptr<Traveler> createPlayerTraveler(size_t nId, std::string n);
    void pickTown(Traveler *t, size_t tIdx);
};

#endif
