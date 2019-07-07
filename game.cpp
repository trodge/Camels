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

#include "game.hpp"

SDL_Texture *textureFromSurfaceSection(SDL_Renderer *rdr, SDL_Surface *sf, const SDL_Rect &rt) {
    int bpp = sf->format->BytesPerPixel;
    Uint8 *p = static_cast<Uint8 *>(sf->pixels) + rt.y * sf->pitch + rt.x * bpp;
    SDL_Surface *c = SDL_CreateRGBSurfaceWithFormatFrom(p, rt.w, rt.h, 32, sf->pitch, SDL_PIXELFORMAT_RGB24);
    if (!c)
        std::cout << "Surface is null at " << rt.x << "," << rt.y << " " << rt.w << "x" << rt.h << " bpp:" << bpp
                  << " SDL Error: " << SDL_GetError() << std::endl;
    SDL_Texture *t = SDL_CreateTextureFromSurface(rdr, c);
    if (!t)
        std::cout << "Failed to create clipped texture, SDL Error:" << SDL_GetError() << std::endl;
    SDL_FreeSurface(c);
    return t;
}

Game::Game() {
    player = std::make_unique<Player>(*this);
    Settings::load("settings.ini");
    loadDisplayVariables();
    player->start();
    window = SDL_CreateWindow("Camels", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, screenRect.w, screenRect.h,
                              SDL_WINDOW_BORDERLESS);
    if (!window)
        std::cout << "Window creation failed, SDL Error:" << SDL_GetError() << std::endl;
    fs::path iconPath("images/icon.png");
    SDL_Surface *icon = IMG_Load(iconPath.string().c_str());
    if (!icon)
        std::cout << "Failed to load icon, IMG Error:" << IMG_GetError() << std::endl;
    SDL_SetWindowIcon(window, icon);
    SDL_FreeSurface(icon);
    screen = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
    if (!screen)
        std::cout << "Failed to create renderer, SDL Error:" << SDL_GetError() << std::endl;
    if (SDL_GetRendererInfo(screen, &screenInfo) < 0)
        std::cout << "Failed to get renderer info, SDL Error:" << SDL_GetError() << std::endl;
    // For debugging, pretend renderer can't handle textures over 64x64
    //     screenInfo.max_texture_height = 64;
    //     screenInfo.max_texture_width = 64;
    fs::path mapPath("images/map-scaled.png");
    mapSurface = IMG_Load(mapPath.string().c_str());
    if (!mapSurface)
        std::cout << "Failed to load map surface, IMG Error:" << IMG_GetError() << std::endl;
    SDL_Rect rt = {0, 0, screenInfo.max_texture_width, screenInfo.max_texture_height};
    mapTextureColumnCount = mapSurface->w / rt.w + (mapSurface->w % rt.w != 0);
    mapTextureRowCount = mapSurface->h / rt.h + (mapSurface->h % rt.h != 0);
    mapTextures.reserve(static_cast<size_t>(mapTextureColumnCount * mapTextureRowCount));
    for (; rt.y + rt.h <= mapSurface->h; rt.y += rt.h) {
        // Loop from top to bottom.
        for (rt.x = 0; rt.x + rt.w <= mapSurface->w; rt.x += rt.w)
            // Loop from left to right.
            mapTextures.push_back(textureFromSurfaceSection(screen, mapSurface, rt));
        if (rt.x < mapSurface->w) {
            // Fill right side rectangles of map.
            rt.w = mapSurface->w - rt.x;
            mapTextures.push_back(textureFromSurfaceSection(screen, mapSurface, rt));
            rt.w = screenInfo.max_texture_width;
        }
    }
    if (rt.y < mapSurface->h) {
        // Fill in bottom side rectangles of map.
        rt.h = mapSurface->h - rt.y;
        for (; rt.x + rt.w <= mapSurface->w; rt.x += rt.w)
            mapTextures.push_back(textureFromSurfaceSection(screen, mapSurface, rt));
        if (rt.x < mapSurface->w) {
            // Fill bottom right corner of map.
            rt.w = mapSurface->w - rt.x;
            mapTextures.push_back(textureFromSurfaceSection(screen, mapSurface, rt));
        }
    }
    mapTexture = SDL_CreateTexture(screen, SDL_PIXELFORMAT_RGB24, SDL_TEXTUREACCESS_TARGET, mapRect.w, mapRect.h);
    renderMapTexture();
    /*SDL_CreateRGBSurfaceWithFormat(
    0, 14220, 6640, screen->format->BitsPerPixel, screen->format->format);
    SDL_BlitScaled(mapOriginal, NULL, mapImage, NULL);*/
    SDL_ShowWindow(window);
    SDL_RaiseWindow(window);
    while (not player->getStop()) {
        handleEvents();
        update();
        draw();
        SDL_Delay(20);
    }
    Settings::save("settings.ini");
}

Game::~Game() {
    std::cout << "Freeing map surface" << std::endl;
    if (mapSurface)
        SDL_FreeSurface(mapSurface);
    mapSurface = nullptr;
    std::cout << "Freeing map textures" << std::endl;
    for (auto &mT : mapTextures) {
        if (mT)
            SDL_DestroyTexture(mT);
        mT = nullptr;
    }
    std::cout << "Freeing map texture" << std::endl;
    if (mapTexture)
        SDL_DestroyTexture(mapTexture);
    mapTexture = nullptr;
    std::cout << "Freeing good images" << std::endl;
    for (auto &gIs : gameData.goodImages)
        for (auto &gI : gIs)
            if (gI.second)
                SDL_FreeSurface(gI.second);
    std::cout << "Destroying screen" << std::endl;
    SDL_DestroyRenderer(screen);
    std::cout << "Destroying window" << std::endl;
    SDL_DestroyWindow(window);
}

void Game::loadDisplayVariables() {
    // Load screen height and width and starting offsets and scale from settings.
    screenRect = Settings::getScreenRect();
    mapRect = Settings::getMapRect();
    offsetX = Settings::getOffsetX();
    offsetY = Settings::getOffsetY();
    scale = Settings::getScale();
}

void Game::renderMapTexture() {
    // Construct map texture for drawing from matrix of map textures.
    int left = mapRect.x / screenInfo.max_texture_width, top = mapRect.y / screenInfo.max_texture_height,
        right = (mapRect.x + mapRect.w) / screenInfo.max_texture_width,
        bottom = (mapRect.y + mapRect.h) / screenInfo.max_texture_height;
    SDL_Rect sR = {0, 0, 0, 0}, dR = {0, 0, 0, 0};
    if (SDL_SetRenderTarget(screen, mapTexture) < 0)
        std::cout << "Failed to set render target, SDL Error: " << SDL_GetError() << std::endl;
    SDL_RenderClear(screen);
    for (int c = left; c <= right; ++c) {
        dR.y = 0;
        dR.h = 0;
        for (int r = top; r <= bottom; ++r) {
            sR = {std::max(mapRect.x - c * screenInfo.max_texture_width, 0),
                  std::max(mapRect.y - r * screenInfo.max_texture_height, 0),
                  std::min(std::min((c + 1) * screenInfo.max_texture_width - mapRect.x, screenInfo.max_texture_width),
                           mapRect.w),
                  std::min(std::min((r + 1) * screenInfo.max_texture_height - mapRect.y, screenInfo.max_texture_height),
                           mapRect.h)};
            dR.y += dR.h;
            dR.w = sR.w;
            dR.h = sR.h;
            SDL_RenderCopy(screen, mapTextures[static_cast<size_t>(r * mapTextureColumnCount + c)], &sR, &dR);
        }
        dR.x += dR.w;
    }
    SDL_SetRenderTarget(screen, nullptr);
}

void Game::place() {
    // Place towns and travelers based on offsets and scale
    std::vector<SDL_Rect> newDrawn;
    newDrawn.reserve(towns.size());
    for (auto &t : towns)
        t.placeDot(newDrawn, offsetX, offsetY, scale);
    for (auto &t : towns)
        t.placeText(newDrawn);
    for (auto &t : aITravelers)
        t->place(offsetX, offsetY, scale);
    player->place(offsetX, offsetY, scale);
}

void Game::moveView(int dx, int dy) {
    // Move view around world map.
    SDL_Rect m = {mapRect.x + dx, mapRect.y + dy, mapSurface->w, mapSurface->h};
    if (m.x > 0 and m.x < m.w - mapRect.w and m.y > 0 and m.y < m.h - mapRect.h) {
        mapRect.x = m.x;
        mapRect.y = m.y;
        offsetX -= dx;
        offsetY -= dy;
        renderMapTexture();
        place();
    }
}

void Game::loadNations(sqlite3 *c) {
    // Load nations and goods data from sqlite database.
    // Load goods.
    std::vector<Good> goods;
    sqlite3_stmt *quer;
    sqlite3_prepare_v2(c, "SELECT * FROM goods", -1, &quer, nullptr);
    while (sqlite3_step(quer) != SQLITE_DONE)
        goods.push_back(Good(static_cast<unsigned int>(sqlite3_column_int(quer, 0)),
                             std::string(reinterpret_cast<const char *>(sqlite3_column_text(quer, 1))),
                             sqlite3_column_double(quer, 2), sqlite3_column_double(quer, 3),
                             std::string(reinterpret_cast<const char *>(sqlite3_column_text(quer, 4)))));
    sqlite3_finalize(quer);
    sqlite3_prepare_v2(c, "SELECT * FROM materials", -1, &quer, nullptr);
    while (sqlite3_step(quer) != SQLITE_DONE) {
        unsigned int g = static_cast<unsigned int>(sqlite3_column_int(quer, 0));
        unsigned int m = static_cast<unsigned int>(sqlite3_column_int(quer, 1));
        goods[g].addMaterial(Material(m, goods[m].getName()));
    }
    sqlite3_finalize(quer);
    // Load good images.
    int tradeHeight = screenRect.h * 29 / Settings::getGoodButtonYDivisor() - 2 * Settings::getTradeBorder();
    SDL_Rect rt = {0, 0, tradeHeight, tradeHeight};
    gameData.goodImages.resize(goods.size());
    for (size_t i = 0; i < goods.size(); ++i) {
        auto &g = goods[i];
        for (auto &m : g.getMaterials()) {
            fs::path imagePath("images/" + m.getName() + "-" + g.getName() + ".png");
            SDL_Surface *image, *scaledImage = nullptr;
            if (fs::exists(imagePath)) {
                image = IMG_Load(imagePath.string().c_str());
                scaledImage = SDL_CreateRGBSurface(0u, rt.w, rt.h, 32, rmask, gmask, bmask, amask);
                SDL_BlitScaled(image, nullptr, scaledImage, &rt);
                SDL_FreeSurface(image);
            }
            gameData.goodImages[i].emplace_hint(gameData.goodImages[i].end(), m.getId(), scaledImage);
        }
    }
    // Load combat stats.
    std::vector<std::unordered_map<unsigned int, std::vector<CombatStat>>> combatStats(goods.size());
    sqlite3_prepare_v2(c, "SELECT * FROM combat_stats", -1, &quer, nullptr);
    while (sqlite3_step(quer) != SQLITE_DONE) {
        unsigned int g = static_cast<unsigned int>(sqlite3_column_int(quer, 0));
        unsigned int m = static_cast<unsigned int>(sqlite3_column_int(quer, 1));
        combatStats[g][m].push_back({static_cast<unsigned int>(sqlite3_column_int(quer, 2)),
                                     static_cast<unsigned int>(sqlite3_column_int(quer, 3)),
                                     static_cast<unsigned int>(sqlite3_column_int(quer, 4)),
                                     static_cast<unsigned int>(sqlite3_column_int(quer, 5)),
                                     static_cast<unsigned int>(sqlite3_column_int(quer, 6)),
                                     {{static_cast<unsigned int>(sqlite3_column_int(quer, 7)),
                                       static_cast<unsigned int>(sqlite3_column_int(quer, 8)),
                                       static_cast<unsigned int>(sqlite3_column_int(quer, 9))}}});
    }
    sqlite3_finalize(quer);
    for (size_t i = 0; i < goods.size(); ++i)
        goods[i].setCombatStats(combatStats[i]);
    // Load parts.
    sqlite3_prepare_v2(c, "SELECT * FROM parts", -1, &quer, nullptr);
    while (sqlite3_step(quer) != SQLITE_DONE)
        gameData.parts.push_back(std::string(reinterpret_cast<const char *>(sqlite3_column_text(quer, 1))));
    sqlite3_finalize(quer);
    // Load statuses.
    sqlite3_prepare_v2(c, "SELECT * FROM statuses", -1, &quer, nullptr);
    while (sqlite3_step(quer) != SQLITE_DONE)
        gameData.statuses.push_back(std::string(reinterpret_cast<const char *>(sqlite3_column_text(quer, 1))));
    sqlite3_finalize(quer);
    // Load combat odds.
    sqlite3_prepare_v2(c, "SELECT * FROM combat_odds", -1, &quer, nullptr);
    while (sqlite3_step(quer) != SQLITE_DONE)
        gameData.odds.push_back({sqlite3_column_double(quer, 1),
                                 {{{sqlite3_column_int(quer, 2), sqlite3_column_double(quer, 3)},
                                   {sqlite3_column_int(quer, 4), sqlite3_column_double(quer, 5)},
                                   {sqlite3_column_int(quer, 6), sqlite3_column_double(quer, 7)}}}});
    sqlite3_finalize(quer);
    sqlite3_prepare_v2(c, "SELECT * FROM town_type_nouns", -1, &quer, nullptr);
    while (sqlite3_step(quer) != SQLITE_DONE)
        gameData.townTypeNouns.push_back(std::string(reinterpret_cast<const char *>(sqlite3_column_text(quer, 1))));
    sqlite3_finalize(quer);
    sqlite3_prepare_v2(c, "SELECT * FROM population_adjectives", -1, &quer, nullptr);
    while (sqlite3_step(quer) != SQLITE_DONE)
        gameData.populationAdjectives.emplace(std::make_pair(
            sqlite3_column_int(quer, 0), std::string(reinterpret_cast<const char *>(sqlite3_column_text(quer, 1)))));
    // Load nations.
    sqlite3_prepare_v2(c, "SELECT * FROM nations", -1, &quer, nullptr);
    while (sqlite3_step(quer) != SQLITE_DONE)
        nations.push_back(Nation(quer, goods));
    sqlite3_finalize(quer);
    for (auto &n : nations)
        n.loadData(c);
}

void Game::newGame() {
    // Load data for a new game from sqlite database.
    sqlite3 *conn;
    if (sqlite3_open("1025ad.db", &conn) != SQLITE_OK)
        std::cout << "Error opening sqlite database:" << sqlite3_errmsg(conn) << std::endl;
    loadNations(conn);
    // Load towns from sqlite database.
    const SDL_Color &fgr = Settings::getLoadBarColor(), &bgr = Settings::getUIForeground();
    int b = Settings::getBigBoxBorder(), r = Settings::getBigBoxRadius(), fS = Settings::getLoadBarFontSize();
    LoadBar loadBar({screenRect.w / 15, screenRect.h * 7 / 15, screenRect.w * 13 / 15, screenRect.h / 15},
                    {"Loading towns..."}, fgr, bgr, b, r, fS);
    // Load businesses.
    sqlite3_stmt *quer;
    sqlite3_prepare_v2(conn, "SELECT modes FROM businesses", -1, &quer, nullptr);
    std::vector<unsigned int> businessModes;
    while (sqlite3_step(quer) != SQLITE_DONE)
        businessModes.push_back(static_cast<unsigned int>(sqlite3_column_int(quer, 0)));
    sqlite3_finalize(quer);
    businesses.clear();
    businesses.reserve(static_cast<size_t>(std::accumulate(businessModes.begin(), businessModes.end(), 0)));
    unsigned int bId = 0;
    for (auto &bM : businessModes) {
        ++bId;
        for (unsigned int m = 1; m <= bM; ++m)
            businesses.push_back(Business(bId, m, conn));
    }
    std::map<std::pair<int, int>, double> frequencyFactors;
    sqlite3_prepare_v2(conn, "SELECT * FROM frequency_factors", -1, &quer, nullptr);

    while (sqlite3_step(quer) != SQLITE_DONE)
        frequencyFactors.emplace(std::make_pair(sqlite3_column_int(quer, 0), sqlite3_column_int(quer, 1)),
                                 sqlite3_column_double(quer, 2));
    sqlite3_finalize(quer);
    sqlite3_prepare_v2(conn, "SELECT COUNT(*) FROM towns", -1, &quer, nullptr);
    sqlite3_step(quer);
    // Store number of towns as double for progress bar purposes.
    double tC = static_cast<double>(sqlite3_column_int(quer, 0));
    sqlite3_finalize(quer);
    sqlite3_prepare_v2(conn, "SELECT * FROM towns", -1, &quer, nullptr);
    towns.reserve(static_cast<size_t>(tC));
    SDL_Surface *freezeSurface = SDL_CreateRGBSurface(0u, screenRect.w, screenRect.h, 32, rmask, gmask, bmask, amask);
    SDL_RenderReadPixels(screen, nullptr, freezeSurface->format->format, freezeSurface->pixels, freezeSurface->pitch);
    SDL_Texture *freezeTexture = SDL_CreateTextureFromSurface(screen, freezeSurface);
    while (sqlite3_step(quer) != SQLITE_DONE and towns.size() < kMaxTowns) {
        SDL_RenderCopy(screen, freezeTexture, nullptr, nullptr);
        towns.push_back(Town(quer, nations, businesses, frequencyFactors, Settings::getTownFontSize()));
        loadBar.progress(1. / tC);
        loadBar.draw(screen);
        SDL_RenderPresent(screen);
    }
    sqlite3_finalize(quer);
    place();
    loadBar.progress(-1);
    loadBar.setText(0, "Finalizing towns...");
    std::vector<std::vector<size_t>> routes(towns.size());
    sqlite3_prepare_v2(conn, "SELECT * FROM routes", -1, &quer, nullptr);
    while (sqlite3_step(quer) != SQLITE_DONE)
        routes[static_cast<size_t>(sqlite3_column_int(quer, 0) - 1)].push_back(
            static_cast<size_t>(sqlite3_column_int(quer, 1)));
    sqlite3_finalize(quer);
    sqlite3_close(conn);
    for (auto &t : towns) {
        SDL_RenderCopy(screen, freezeTexture, nullptr, nullptr);
        t.loadNeighbors(towns, routes[t.getId() - 1]);
        t.update(Settings::getBusinessRunTime());
        loadBar.progress(1. / tC);
        loadBar.draw(screen);
        SDL_RenderPresent(screen);
    }
    loadBar.progress(-1);
    loadBar.setText(0, "Generating Travelers...");
    for (auto &t : towns) {
        SDL_RenderCopy(screen, freezeTexture, nullptr, nullptr);
        t.generateTravelers(gameData, aITravelers);
        loadBar.setText(0, "Generating Travelers..." + std::to_string(aITravelers.size()));
        loadBar.progress(1. / tC);
        loadBar.draw(screen);
        SDL_RenderPresent(screen);
    }
    loadBar.progress(-1);
    tC = static_cast<double>(aITravelers.size());
    loadBar.setText(0, "Starting AI...");
    for (auto &t : aITravelers) {
        SDL_RenderCopy(screen, freezeTexture, nullptr, nullptr);
        t->startAI();
        t->addToTown();
        loadBar.progress(1. / tC);
        loadBar.draw(screen);
        SDL_RenderPresent(screen);
    }
}

void Game::loadGame(const fs::path &p) {
    // Load a saved game from the given path.
    std::ifstream file(p.string(), std::ifstream::binary);
    if (file.is_open()) {
        sqlite3 *conn;
        if (sqlite3_open("1025ad.db", &conn) != SQLITE_OK)
            std::cout << sqlite3_errmsg(conn) << std::endl;
        nations.clear();
        loadNations(conn);
        sqlite3_close(conn);
        aITravelers.clear();
        towns.clear();
        file.seekg(0, file.end);
        std::streamsize length = file.tellg();
        file.seekg(0, file.beg);
        char *buffer = new char[static_cast<size_t>(length)];
        file.read(buffer, length);
        auto game = Save::GetGame(buffer);
        auto lTowns = game->towns();
        LoadBar loadBar({screenRect.w / 15, screenRect.h * 7 / 15, screenRect.w * 13 / 15, screenRect.h / 15},
                        {"Loading towns..."}, Settings::getLoadBarColor(), Settings::getUIForeground(),
                        Settings::getBigBoxBorder(), Settings::getBigBoxRadius(), Settings::getLoadBarFontSize());
        towns.reserve(lTowns->size());
        for (auto lTI = lTowns->begin(); lTI != lTowns->end(); ++lTI) {
            towns.push_back(Town(*lTI, nations, Settings::getTownFontSize()));
            loadBar.progress(1. / lTowns->size());
            loadBar.draw(screen);
            SDL_RenderPresent(screen);
        }
        loadBar.progress(-1);
        loadBar.setText(0, "Finalizing towns...");
        for (flatbuffers::uoffset_t i = 0; i < towns.size(); ++i) {
            std::vector<size_t> neighborIds(lTowns->Get(i)->neighbors()->begin(), lTowns->Get(i)->neighbors()->end());
            towns[i].loadNeighbors(towns, neighborIds);
            loadBar.progress(1. / static_cast<double>(towns.size()));
            loadBar.draw(screen);
            SDL_RenderPresent(screen);
        }
        auto lTravelers = game->aITravelers();
        auto lTI = lTravelers->begin();
        player->loadTraveler(*lTI, towns);
        for (++lTI; lTI != lTravelers->end(); ++lTI)
            aITravelers.push_back(std::make_shared<Traveler>(*lTI, towns, nations, gameData));
        for (auto &t : aITravelers)
            t->startAI();
        place();
    }
}

void Game::handleEvents() {
    // Handle events since last poll.
    SDL_Event event;
    while (SDL_PollEvent(&event)) {
        Uint8 r, g, b;
        int mx, my, mw, mh;
        switch (event.type) {
        case SDL_KEYDOWN:
            switch (event.key.keysym.sym) {
            case SDLK_u:
                --offsetY;
                place();
                std::cout << "Y offset decreased" << std::endl;
                break;
            case SDLK_j:
                ++offsetY;
                place();
                std::cout << "Y offset increased" << std::endl;
                break;
            case SDLK_h:
                --offsetX;
                place();
                std::cout << "X offset decreased" << std::endl;
                break;
            case SDLK_k:
                ++offsetX;
                place();
                std::cout << "X offset increased" << std::endl;
                break;
            }
            break;
        case SDL_MOUSEBUTTONDOWN:
            // Print r g b values at clicked coordinates
            mx = event.button.x + mapRect.x;
            my = event.button.y + mapRect.y;
            mw = mapSurface->w;
            mh = mapSurface->h;
            if (mx >= 0 and mx < mw and my >= 0 and my < mh) {
                SDL_GetRGB(getAt(mapSurface, mx, my), mapSurface->format, &r, &g, &b);
                std::cout << '(' << int(r) << ',' << int(g) << ',' << int(b) << ')' << std::endl;
            }
            break;
        }
        player->handleEvent(event);
    }
}

void Game::update() {
    currentTime = SDL_GetTicks();
    unsigned int elapsed = currentTime - lastTime;
    lastTime = currentTime;
    if (not(player->getPause())) {
        for (auto &t : towns)
            t.update(elapsed);
        for (auto &t : aITravelers) {
            t->runAI(elapsed);
            t->update(elapsed);
            t->place(offsetX, offsetY, scale);
        }
    }
    player->update(elapsed);
    player->place(offsetX, offsetY, scale);
}

void Game::draw() {
    // Draw UI and game elements.
    SDL_RenderCopy(screen, mapTexture, nullptr, nullptr);
    for (auto &t : towns)
        t.drawRoutes(screen);
    for (auto &t : towns)
        t.drawDot(screen);
    for (auto &t : aITravelers)
        t->draw(screen);
    for (auto &t : towns)
        t.drawText(screen);
    player->draw(screen);
    SDL_RenderPresent(screen);
}

void Game::saveRoutes() {
    // Recalculate routes between towns and save new routes to database.
    LoadBar loadBar({screenRect.w / 15, screenRect.h * 7 / 15, screenRect.w * 13 / 15, screenRect.h / 15},
                    {"Finding routes..."}, {0, 255, 0, 255}, Settings::getUIForeground(), Settings::getBigBoxRadius(),
                    Settings::getBigBoxBorder(), Settings::getLoadBarFontSize());
    for (auto &t : towns) {
        t.findNeighbors(towns, mapSurface, mapRect.x, mapRect.y);
        loadBar.progress(1. / static_cast<double>(towns.size()));
        loadBar.draw(screen);
        SDL_RenderPresent(screen);
    }
    for (auto &t : towns)
        t.connectRoutes();
    std::string inserts = "INSERT OR IGNORE INTO routes VALUES";
    for (auto &t : towns)
        t.saveNeighbors(inserts);
    inserts.pop_back();
    sqlite3 *conn;
    if (sqlite3_open("1025ad.db", &conn) != SQLITE_OK)
        std::cout << "Error opening sqlite database:" << sqlite3_errmsg(conn) << std::endl;
    sqlite3_stmt *comm;
    sqlite3_prepare_v2(conn, inserts.c_str(), -1, &comm, nullptr);
    sqlite3_step(comm);
    sqlite3_finalize(comm);
    sqlite3_close(conn);
}

void Game::saveData() {
    // Save changes to frequencies and demand slopes to database.
    sqlite3 *conn;
    if (sqlite3_open("1025ad.db", &conn) != SQLITE_OK)
        std::cout << "Error opening sqlite database:" << sqlite3_errmsg(conn) << std::endl;
    std::string updates = "UPDATE frequencies SET frequency = CASE";
    player->getTown()->saveFrequencies(updates);
    sqlite3_stmt *comm;
    sqlite3_prepare_v2(conn, updates.c_str(), -1, &comm, nullptr);
    if (sqlite3_step(comm) != SQLITE_DONE)
        std::cout << "Error updating frequencies: " << sqlite3_errmsg(conn) << std::endl << updates << std::endl;
    sqlite3_finalize(comm);
    updates = "UPDATE consumption SET demand_slope = CASE";
    player->getTown()->saveDemand(updates);
    sqlite3_prepare_v2(conn, updates.c_str(), -1, &comm, nullptr);
    if (sqlite3_step(comm) != SQLITE_DONE)
        std::cout << "Error updating demand slopes: " << sqlite3_errmsg(conn) << std::endl << updates << std::endl;
    sqlite3_finalize(comm);
    if (sqlite3_close(conn) != SQLITE_OK)
        std::cout << sqlite3_errmsg(conn) << std::endl;
}

void Game::saveGame() {
    // Save the game.
    if (not player->hasTraveler())
        std::cout << "Tried to save game with no player traveler" << std::endl;
    flatbuffers::FlatBufferBuilder builder(1024);
    auto sTowns = builder.CreateVector<flatbuffers::Offset<Save::Town>>(
        towns.size(), [this, &builder](size_t i) { return towns[i].save(builder); });
    auto sAITravelers = builder.CreateVector<flatbuffers::Offset<Save::Traveler>>(
        aITravelers.size(), [this, &builder](size_t i) { return aITravelers[i]->save(builder); });
    auto game = Save::CreateGame(builder, sTowns, player->save(builder), sAITravelers);
    builder.Finish(game);
    fs::path path("save");
    path /= aITravelers.front()->getName();
    path.replace_extension("sav");
    std::ofstream file(path.string(), std::ofstream::binary);
    if (file.is_open())
        file.write((const char *)builder.GetBufferPointer(), builder.GetSize());
}

void Game::fillFocusableTowns(std::vector<Focusable *> &fcbls) {
    // Fill the focusables vector with towns.
    for (auto &t : towns)
        fcbls.push_back(&t);
}

std::shared_ptr<Traveler> Game::createPlayerTraveler(size_t nId, std::string n) {
    if (n.empty())
        n = nations[nId].randomName();
    // Create traveler object for player
    auto traveler = std::make_shared<Traveler>(n, &towns[nId - 1], gameData);
    traveler->addToTown();
    traveler->place(offsetX, offsetY, scale);
    return traveler;
}

void Game::pickTown(std::shared_ptr<Traveler> t, size_t tId) { t->pickTown(&towns[tId]); }
