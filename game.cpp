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

Game::Game()
    : screenRect(Settings::getScreenRect()), mapRect(Settings::getMapRect()), offsetX(Settings::getOffsetX()),
      offsetY(Settings::getOffsetY()), scale(Settings::getScale()),
      window(sdl::makeWindow("Camels", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, screenRect.w, screenRect.h,
                             SDL_WINDOW_BORDERLESS)),
      screen(sdl::makeRenderer(window.get(), -1, SDL_RENDERER_ACCELERATED)) {
    player = std::make_unique<Player>(*this);
    player->start();
    std::cout << "Creating Game" << std::endl;
    if (!window.get())
        std::cout << "Window creation failed, SDL Error:" << SDL_GetError() << std::endl;
    sdl::SurfacePtr icon = sdl::loadImage(fs::path("images/icon.png").string().c_str());
    if (!icon.get())
        std::cout << "Failed to load icon, IMG Error:" << IMG_GetError() << std::endl;
    SDL_SetWindowIcon(window.get(), icon.get());
    icon = nullptr;
    if (!screen.get())
        std::cout << "Failed to create renderer, SDL Error:" << SDL_GetError() << std::endl;
    if (SDL_GetRendererInfo(screen.get(), &screenInfo) < 0)
        std::cout << "Failed to get renderer info, SDL Error:" << SDL_GetError() << std::endl;
    // For debugging, pretend renderer can't handle textures over 64x64
    // screenInfo.max_texture_height = 64;
    // screenInfo.max_texture_width = 64;
    std::cout << "Loading Map" << std::endl;
    mapSurface = sdl::loadImage(fs::path("images/map-scaled.png").string().c_str());
    mapTexture = sdl::makeTexture(screen.get(), SDL_PIXELFORMAT_RGB24, SDL_TEXTUREACCESS_TARGET, mapRect.w, mapRect.h);
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
            mapTextures.push_back(sdl::makeTextureFromSurfaceSection(screen.get(), mapSurface.get(), rt));
        if (rt.x < mapSurface->w) {
            // Fill right side rectangles of map.
            rt.w = mapSurface->w - rt.x;
            mapTextures.push_back(sdl::makeTextureFromSurfaceSection(screen.get(), mapSurface.get(), rt));
            rt.w = screenInfo.max_texture_width;
        }
    }
    if (rt.y < mapSurface->h) {
        // Fill in bottom side rectangles of map.
        rt.h = mapSurface->h - rt.y;
        for (; rt.x + rt.w <= mapSurface->w; rt.x += rt.w)
            mapTextures.push_back(sdl::makeTextureFromSurfaceSection(screen.get(), mapSurface.get(), rt));
        if (rt.x < mapSurface->w) {
            // Fill bottom right corner of map.
            rt.w = mapSurface->w - rt.x;
            mapTextures.push_back(sdl::makeTextureFromSurfaceSection(screen.get(), mapSurface.get(), rt));
        }
    }
    renderMapTexture();
    /*SDL_CreateRGBSurfaceWithFormat(
    0, 14220, 6640, screen->format->BitsPerPixel, screen->format->format);
    SDL_BlitScaled(mapOriginal, NULL, mapImage, NULL);*/
    SDL_ShowWindow(window.get());
    SDL_RaiseWindow(window.get());
}

Game::~Game() {
    std::cout << "Freeing map surface" << std::endl;
    mapSurface = nullptr;
    std::cout << "Freeing map textures" << std::endl;
    for (auto &mT : mapTextures)
        mT = nullptr;
    std::cout << "Freeing map texture" << std::endl;
    mapTexture = nullptr;
    std::cout << "Freeing good images" << std::endl;
    for (auto &gI : goodImages)
        gI = nullptr;
    std::cout << "Destroying screen" << std::endl;
    screen = nullptr;
    std::cout << "Destroying window" << std::endl;
    window = nullptr;
}

void Game::run() {
    // Run the game loop.
    while (not player->getStop()) {
        handleEvents();
        update();
        draw();
        SDL_Delay(20);
    }
}

void Game::renderMapTexture() {
    // Construct map texture for drawing from matrix of map textures.
    int left = mapRect.x / screenInfo.max_texture_width, top = mapRect.y / screenInfo.max_texture_height,
        right = (mapRect.x + mapRect.w) / screenInfo.max_texture_width,
        bottom = (mapRect.y + mapRect.h) / screenInfo.max_texture_height;
    SDL_Rect sR = {0, 0, 0, 0}, dR = {0, 0, 0, 0};
    if (SDL_SetRenderTarget(screen.get(), mapTexture.get()) < 0)
        std::cout << "Failed to set render target, SDL Error: " << SDL_GetError() << std::endl;
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
            SDL_RenderCopy(screen.get(), mapTextures[static_cast<size_t>(r * mapTextureColumnCount + c)].get(), &sR, &dR);
        }
        dR.x += dR.w;
    }
    SDL_SetRenderTarget(screen.get(), nullptr);
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

void Game::loadData(sqlite3 *cn) {
    // Load data from database which is needed both for new game and loading a game.
    sql::StmtPtr quer;
    std::cout << "Loading data" << std::endl;
    // Load game data.
    // Load part names.
    quer = sql::makeQuery(cn, "SELECT name FROM parts");
    while (sqlite3_step(quer.get()) != SQLITE_DONE)
        gameData.parts.push_back(std::string(reinterpret_cast<const char *>(sqlite3_column_text(quer.get(), 0))));
    
    // Load status names.
    quer = sql::makeQuery(cn, "SELECT name FROM statuses");
    while (sqlite3_step(quer.get()) != SQLITE_DONE)
        gameData.statuses.push_back(std::string(reinterpret_cast<const char *>(sqlite3_column_text(quer.get(), 0))));
    
    // Load combat odds.
    quer = sql::makeQuery(cn,
                       "SELECT hit_odds, status_1, status_1_chance, status_2, "
                       "status_2_chance, status_3, status_3_chance FROM combat_odds");
    while (sqlite3_step(quer.get()) != SQLITE_DONE)
        gameData.odds.push_back({sqlite3_column_double(quer.get(), 0),
                                 {{{sqlite3_column_int(quer.get(), 1), sqlite3_column_double(quer.get(), 2)},
                                   {sqlite3_column_int(quer.get(), 3), sqlite3_column_double(quer.get(), 4)},
                                   {sqlite3_column_int(quer.get(), 5), sqlite3_column_double(quer.get(), 6)}}}});
    
    // Load town type nouns.
    quer = sql::makeQuery(cn, "SELECT noun FROM town_type_nouns");
    while (sqlite3_step(quer.get()) != SQLITE_DONE)
        gameData.townTypeNouns.push_back(std::string(reinterpret_cast<const char *>(sqlite3_column_text(quer.get(), 0))));
    
    quer = sql::makeQuery(cn, "SELECT minimum, adjective FROM population_adjectives");
    while (sqlite3_step(quer.get()) != SQLITE_DONE)
        gameData.populationAdjectives.emplace(std::make_pair(
            sqlite3_column_int(quer.get(), 0), std::string(reinterpret_cast<const char *>(sqlite3_column_text(quer.get(), 1)))));
    

    // Load goods.
    std::vector<Good> goods;
    quer = sql::makeQuery(cn, "SELECT good_id, name, perish, carry, measure, shoots FROM goods");
    while (sqlite3_step(quer.get()) != SQLITE_DONE)
        goods.push_back(Good(static_cast<unsigned int>(sqlite3_column_int(quer.get(), 0)),
                             std::string(reinterpret_cast<const char *>(sqlite3_column_text(quer.get(), 1))),
                             sqlite3_column_double(quer.get(), 2), sqlite3_column_double(quer.get(), 3),
                             std::string(reinterpret_cast<const char *>(sqlite3_column_text(quer.get(), 4))),
                             static_cast<unsigned int>(sqlite3_column_int(quer.get(), 5))));
    

    // Load materials for goods.
    quer = sql::makeQuery(cn, "SELECT good_id, material_id FROM materials");
    while (sqlite3_step(quer.get()) != SQLITE_DONE)
        goods[static_cast<size_t>(sqlite3_column_int(quer.get(), 0))].addMaterial(
            goods[static_cast<unsigned int>(sqlite3_column_int(quer.get(), 1))]);
    

    // Load good images.
    int imageSize = screenRect.h * kGoodButtonSizeMultiplier / kGoodButtonYDivisor - 2 * Settings::getTradeBorder();
    SDL_Rect rt = {0, 0, imageSize, imageSize};
    goodImages.reserve(goods.size());
    for (size_t i = 0; i < goods.size(); ++i) {
        // Loop over goods by index.
        auto &g = goods[i];
        auto &gMs = goods[i].getMaterials();
        for (size_t j = 0; j < gMs.size(); ++j) {
            // Loop over materials by index.
            auto &m = gMs[j];
            std::string name = g.getFullName(m);
            // Replace a space in the good's full name with a dash.
            size_t spacePos = name.find(' ');
            if (spacePos != std::string::npos)
                name.replace(spacePos, 1u, "-");
            // Concatenate material name with good name.
            fs::path imagePath("images/" + name + ".png");
            if (fs::exists(imagePath)) {
                // Load image from file.
                sdl::SurfacePtr image(sdl::loadImage(imagePath.string().c_str()));
                // Put empty surface in vector.
                goodImages.push_back(sdl::makeSurface(rt.w, rt.h));
                // Scale image into vector.
                SDL_BlitScaled(image.get(), nullptr, goodImages.back().get(), &rt);
                // Pass pointer to good and material.
                g.setImage(j, goodImages.back().get());
            }
        }
    }

    // Load combat stats.
    std::vector<std::unordered_map<unsigned int, std::vector<CombatStat>>> combatStats(goods.size());
    quer = sql::makeQuery(cn,
                       "SELECT good_id, material_id, stat_id, part_id, attack, type, "
                       "speed, bash_defense, cut_defense, stab_defense FROM combat_stats");
    while (sqlite3_step(quer.get()) != SQLITE_DONE) {
        unsigned int g = static_cast<unsigned int>(sqlite3_column_int(quer.get(), 0));
        unsigned int m = static_cast<unsigned int>(sqlite3_column_int(quer.get(), 1));
        combatStats[g][m].push_back({static_cast<unsigned int>(sqlite3_column_int(quer.get(), 2)),
                                     static_cast<unsigned int>(sqlite3_column_int(quer.get(), 3)),
                                     static_cast<unsigned int>(sqlite3_column_int(quer.get(), 4)),
                                     static_cast<unsigned int>(sqlite3_column_int(quer.get(), 5)),
                                     static_cast<unsigned int>(sqlite3_column_int(quer.get(), 6)),
                                     {{static_cast<unsigned int>(sqlite3_column_int(quer.get(), 7)),
                                       static_cast<unsigned int>(sqlite3_column_int(quer.get(), 8)),
                                       static_cast<unsigned int>(sqlite3_column_int(quer.get(), 9))}}});
    }
    
    for (size_t i = 0u; i < goods.size(); ++i)
        goods[i].setCombatStats(combatStats[i]);

    // Load businesses.
    std::vector<Business> businesses;
    quer = sql::makeQuery(cn,
                       "SELECT business_id, modes, name, can_switch, "
                       "require_coast, keep_material FROM businesses");
    unsigned int bId = 1u;
    while (sqlite3_step(quer.get()) != SQLITE_DONE)
        for (unsigned int bMd = 1u; bMd <= static_cast<size_t>(sqlite3_column_int(quer.get(), 1)); ++bMd)
            businesses.push_back(Business(static_cast<unsigned int>(sqlite3_column_int(quer.get(), 0)), bMd,
                                          std::string(reinterpret_cast<const char *>(sqlite3_column_text(quer.get(), 2))),
                                          static_cast<bool>(sqlite3_column_int(quer.get(), 3)),
                                          static_cast<bool>(sqlite3_column_int(quer.get(), 4)),
                                          static_cast<bool>(sqlite3_column_int(quer.get(), 5))));
    
    // Load requirements.
    quer = sql::makeQuery(cn, "SELECT business_id, good_id, amount FROM requirements");
    std::vector<Good> requirements;
    auto bIt = businesses.begin();
    while (sqlite3_step(quer.get()) != SQLITE_DONE) {
        bId = static_cast<unsigned int>(sqlite3_column_int(quer.get(), 0));
        if (bIt->getId() != bId) {
            // Business ids don't match, flush vector and increment.
            for (; bIt->getId() != bId; ++bIt)
                // Loop over modes.
                bIt->setRequirements(requirements);
            requirements.clear();
        }
        unsigned int gId = static_cast<unsigned int>(sqlite3_column_int(quer.get(), 1));
        requirements.push_back(Good(gId, goods[gId].getName(), sqlite3_column_double(quer.get(), 2), goods[gId].getMeasure()));
    }
    
    // Set requirements for last business.
    for (; bIt != businesses.end(); ++bIt)
        // Loop over modes.
        bIt->setRequirements(requirements);
    // Load inputs.
    quer = sql::makeQuery(cn, "SELECT business_id, mode, good_id, amount FROM inputs");
    std::vector<Good> inputs;
    bIt = businesses.begin();
    while (sqlite3_step(quer.get()) != SQLITE_DONE) {
        if (bIt->getId() != static_cast<unsigned int>(sqlite3_column_int(quer.get(), 0)) or
            bIt->getMode() != static_cast<unsigned int>(sqlite3_column_int(quer.get(), 1))) {
            // Business ids or modes don't match, flush vector and increment
            bIt->setInputs(inputs);
            inputs.clear();
            ++bIt;
        }
        unsigned int gId = static_cast<unsigned int>(sqlite3_column_int(quer.get(), 2));
        inputs.push_back(Good(gId, goods[gId].getName(), sqlite3_column_double(quer.get(), 3), goods[gId].getMeasure()));
    }
    
    // Set inputs for last business.
    bIt->setInputs(inputs);
    // Load outputs.
    quer = sql::makeQuery(cn, "SELECT business_id, mode, good_id, amount FROM outputs");
    std::vector<Good> outputs;
    bIt = businesses.begin();
    while (sqlite3_step(quer.get()) != SQLITE_DONE) {
        if (bIt->getId() != static_cast<unsigned int>(sqlite3_column_int(quer.get(), 0)) or
            bIt->getMode() != static_cast<unsigned int>(sqlite3_column_int(quer.get(), 1))) {
            // Business ids or modes don't match, flush vector and increment
            bIt->setOutputs(outputs);
            outputs.clear();
            ++bIt;
        }
        unsigned int gId = static_cast<unsigned int>(sqlite3_column_int(quer.get(), 2));
        outputs.push_back(Good(gId, goods[gId].getName(), sqlite3_column_double(quer.get(), 3), goods[gId].getMeasure()));
    }
    
    // Set outputs for last business.
    bIt->setOutputs(outputs);

    // Load nations.
    quer = sql::makeQuery(cn,
                       "SELECT nation_id, english_name, language_name, adjective, color_r, "
                       "color_g, color_b,"
                       "background_r, background_g, background_b, religion FROM nations");
    while (sqlite3_step(quer.get()) != SQLITE_DONE)
        nations.push_back(
            Nation(static_cast<unsigned int>(sqlite3_column_int(quer.get(), 0)),
                   {std::string(reinterpret_cast<const char *>(sqlite3_column_text(quer.get(), 1))),
                    std::string(reinterpret_cast<const char *>(sqlite3_column_text(quer.get(), 2)))},
                   std::string(reinterpret_cast<const char *>(sqlite3_column_text(quer.get(), 3))),
                   {static_cast<Uint8>(sqlite3_column_int(quer.get(), 4)), static_cast<Uint8>(sqlite3_column_int(quer.get(), 5)),
                    static_cast<Uint8>(sqlite3_column_int(quer.get(), 6)), 255},
                   {static_cast<Uint8>(sqlite3_column_int(quer.get(), 7)), static_cast<Uint8>(sqlite3_column_int(quer.get(), 8)),
                    static_cast<Uint8>(sqlite3_column_int(quer.get(), 9)), 255},
                   std::string(reinterpret_cast<const char *>(sqlite3_column_text(quer.get(), 10))), goods, businesses));
    

    // Load traveler names into nations.
    quer = sql::makeQuery(cn, "SELECT nation_id, name FROM names");
    size_t ntId = 1;
    std::vector<std::string> travelerNames;
    while (sqlite3_step(quer.get()) != SQLITE_DONE) {
        if (ntId != static_cast<size_t>(sqlite3_column_int(quer.get(), 0))) {
            // Nation index doesn't match, flush vector and increment.
            nations[ntId - 1].setTravelerNames(travelerNames);
            travelerNames.clear();
            ++ntId;
        }
        travelerNames.push_back(std::string(reinterpret_cast<const char *>(sqlite3_column_text(quer.get(), 1))));
    }
    
    // Set traveler names for last nation.
    nations[ntId - 1].setTravelerNames(travelerNames);

    // Load frequencies of businesses into nations.
    quer = sql::makeQuery(cn, "SELECT nation_id, frequency FROM frequencies");
    ntId = 1;
    std::vector<double> frequencies;
    frequencies.reserve(businesses.size());
    while (sqlite3_step(quer.get()) != SQLITE_DONE) {
        if (ntId != static_cast<size_t>(sqlite3_column_int(quer.get(), 0))) {
            // Nation index doesn't match, flush vector and increment.
            nations[ntId - 1].setFrequencies(frequencies);
            frequencies.clear();
            ++ntId;
        }
        frequencies.push_back(sqlite3_column_double(quer.get(), 1));
    }
    
    // Set frequencies for last nation.
    nations[ntId - 1].setFrequencies(frequencies);

    // Load consumption information for each material of each good into nations.
    quer = sql::makeQuery(cn,
                       "SELECT nation_id, good_id, amount, demand_slope, "
                       "demand_intercept FROM consumption");
    ntId = 1;
    size_t gdId = 0;
    std::vector<std::vector<std::array<double, 3>>> goodConsumptions;
    goodConsumptions.reserve(goods.size());
    std::vector<std::array<double, 3>> materialConsumptions;
    while (sqlite3_step(quer.get()) != SQLITE_DONE) {
        if (ntId != static_cast<size_t>(sqlite3_column_int(quer.get(), 0))) {
            // Nation index doesn't match, flush vector, reset good id, and
            // increment.
            nations[ntId - 1].setGoodConsumptions(goodConsumptions);
            goodConsumptions.clear();
            gdId = 0;
            ++ntId;
        }
        if (gdId != static_cast<size_t>(sqlite3_column_int(quer.get(), 1))) {
            // Good index doesn't match, flush vector and increment.
            goodConsumptions.push_back(materialConsumptions);
            materialConsumptions.clear();
            ++gdId;
        }
        materialConsumptions.push_back(
            {{sqlite3_column_double(quer.get(), 2), sqlite3_column_double(quer.get(), 3), sqlite3_column_double(quer.get(), 4)}});
    }
    
}

void Game::loadTowns(sqlite3 *cn, LoadBar &ldBr, SDL_Texture *frzTx) {
    sql::StmtPtr quer;
    // Load frequency factors.
    std::map<std::pair<unsigned int, unsigned int>, double> frequencyFactors; // Keys are town type, business id.
    quer = sql::makeQuery(cn, "SELECT town_type, business_id, factor FROM frequency_factors");
    while (sqlite3_step(quer.get()) != SQLITE_DONE)
        frequencyFactors.emplace(std::make_pair(sqlite3_column_int(quer.get(), 0), sqlite3_column_int(quer.get(), 1)),
                                 sqlite3_column_double(quer.get(), 2));
    

    quer = sql::makeQuery(cn, "SELECT COUNT(*) FROM towns");
    sqlite3_step(quer.get());
    // Game data holds town count for traveler properties.
    gameData.townCount = static_cast<unsigned int>(sqlite3_column_int(quer.get(), 0));
    // Store number of towns as double for progress bar purposes.
    double tC = static_cast<double>(gameData.townCount);
    
    quer = sql::makeQuery(cn,
                       "SELECT town_id, eng, lang, nation_id, latitude, "
                       "longitude, coastal, population,"
                       "town_type FROM towns");
    towns.reserve(gameData.townCount);
    std::cout << "Loading towns" << std::endl;
    while (sqlite3_step(quer.get()) != SQLITE_DONE and towns.size() < kMaxTowns) {
        SDL_RenderCopy(screen.get(), frzTx, nullptr, nullptr);
        std::vector<std::string> names = {std::string(reinterpret_cast<const char *>(sqlite3_column_text(quer.get(), 1))),
                                          std::string(reinterpret_cast<const char *>(sqlite3_column_text(quer.get(), 2)))};
        towns.push_back(Town(static_cast<unsigned int>(sqlite3_column_int(quer.get(), 0)), names,
                             &nations[static_cast<size_t>(sqlite3_column_int(quer.get(), 3) - 1)], sqlite3_column_double(quer.get(), 5),
                             sqlite3_column_double(quer.get(), 4), static_cast<bool>(sqlite3_column_int(quer.get(), 6)),
                             static_cast<unsigned long>(sqlite3_column_int(quer.get(), 7)),
                             static_cast<unsigned int>(sqlite3_column_int(quer.get(), 8)), frequencyFactors,
                             Settings::getTownFontSize(), printer));
        ldBr.progress(1. / tC);
        ldBr.draw(screen.get());
        SDL_RenderPresent(screen.get());
    }
    
    place();
    ldBr.progress(-1);
    ldBr.setText(0, "Finalizing towns...");
    std::vector<unsigned int> neighborIds;
    quer = sql::makeQuery(cn, "SELECT from_id, to_id FROM routes");
    unsigned int tnId = 1;
    while (sqlite3_step(quer.get()) != SQLITE_DONE) {
        if (tnId != static_cast<unsigned int>(sqlite3_column_int(quer.get(), 0))) {
            SDL_RenderCopy(screen.get(), frzTx, nullptr, nullptr);
            // Town ids don't match, flush vector and increment.
            towns[tnId - 1].loadNeighbors(towns, neighborIds);
            neighborIds.clear();
            towns[tnId - 1].update(Settings::getBusinessRunTime());
            ++tnId;
            ldBr.progress(1. / tC);
            ldBr.draw(screen.get());
            SDL_RenderPresent(screen.get());
        }
        neighborIds.push_back(static_cast<unsigned int>(sqlite3_column_int(quer.get(), 1)));
    }
    
    // Load neighbors of final town.
    towns[tnId - 1].loadNeighbors(towns, neighborIds);
}

void Game::newGame() {
    // Load data for a new game from sqlite database.
    sql::DtbsPtr conn = sql::makeConnection(fs::path("1025ad.db"), SQLITE_OPEN_READONLY);
    ;
    loadData(conn.get());

    // Load towns from sqlite database.
    LoadBar loadBar({screenRect.w / 15, screenRect.h * 7 / 15, screenRect.w * 13 / 15, screenRect.h / 15},
                    {"Loading towns..."}, Settings::getLoadBarColor(), Settings::getUIForeground(),
                    Settings::getBigBoxBorder(), Settings::getBigBoxRadius(), Settings::getLoadBarFontSize(), printer);
    // Create a texture for keeping the current screen frozen behind load bar.
    sdl::SurfacePtr freezeSurface = sdl::makeSurface(screenRect.w, screenRect.h);
    SDL_RenderReadPixels(screen.get(), nullptr, freezeSurface->format->format, freezeSurface->pixels, freezeSurface->pitch);
    sdl::TexturePtr freezeTexture = sdl::makeTextureFromSurface(screen.get(), freezeSurface.get());
    freezeSurface = nullptr;
    loadTowns(conn.get(), loadBar, freezeTexture.get());
    conn = nullptr;
    double tC = towns.size();
    loadBar.progress(-1);
    loadBar.setText(0, "Generating Travelers...");
    for (auto &t : towns) {
        SDL_RenderCopy(screen.get(), freezeTexture.get(), nullptr, nullptr);
        t.generateTravelers(gameData, aITravelers);
        loadBar.setText(0, "Generating Travelers..." + std::to_string(aITravelers.size()));
        loadBar.progress(1. / tC);
        loadBar.draw(screen.get());
        SDL_RenderPresent(screen.get());
    }
    loadBar.progress(-1);
    tC = static_cast<double>(aITravelers.size());
    loadBar.setText(0, "Starting AI...");
    for (auto &t : aITravelers) {
        SDL_RenderCopy(screen.get(), freezeTexture.get(), nullptr, nullptr);
        t->startAI();
        t->addToTown();
        loadBar.progress(1. / tC);
        loadBar.draw(screen.get());
        SDL_RenderPresent(screen.get());
    }
}

void Game::loadGame(const fs::path &p) {
    sql::DtbsPtr conn = sql::makeConnection(fs::path("1025ad.db"), SQLITE_OPEN_READONLY);
    nations.clear();
    loadData(conn.get());
    conn = nullptr;
    // Load a saved game from the given path.
    std::ifstream file(p.string(), std::ifstream::binary);
    if (file.is_open()) {
        aITravelers.clear();
        towns.clear();
        file.seekg(0, file.end);
        std::streamsize length = file.tellg();
        file.seekg(0, file.beg);
        char *buffer = new char[static_cast<size_t>(length)];
        file.read(buffer, length);
        auto game = Save::GetGame(buffer);
        auto lTowns = game->towns();
        towns.reserve(lTowns->size());
        LoadBar loadBar({screenRect.w / 15, screenRect.h * 7 / 15, screenRect.w * 13 / 15, screenRect.h / 15},
                        {"Loading towns..."}, Settings::getLoadBarColor(), Settings::getUIForeground(),
                        Settings::getBigBoxBorder(), Settings::getBigBoxRadius(), Settings::getLoadBarFontSize(), printer);
        sdl::SurfacePtr freezeSurface = sdl::makeSurface(screenRect.w, screenRect.h);
        SDL_RenderReadPixels(screen.get(), nullptr, freezeSurface->format->format, freezeSurface->pixels,
                             freezeSurface->pitch);
        sdl::TexturePtr freezeTexture = sdl::makeTextureFromSurface(screen.get(), freezeSurface.get());
        freezeSurface = nullptr;
        for (auto lTI = lTowns->begin(); lTI != lTowns->end(); ++lTI) {
            SDL_RenderCopy(screen.get(), freezeTexture.get(), nullptr, nullptr);
            towns.push_back(Town(*lTI, nations, Settings::getTownFontSize(), printer));
            loadBar.progress(1. / lTowns->size());
            loadBar.draw(screen.get());
            SDL_RenderPresent(screen.get());
        }
        loadBar.progress(-1);
        loadBar.setText(0, "Finalizing towns...");
        for (flatbuffers::uoffset_t i = 0; i < towns.size(); ++i) {
            SDL_RenderCopy(screen.get(), freezeTexture.get(), nullptr, nullptr);
            std::vector<unsigned int> neighborIds(lTowns->Get(i)->neighbors()->begin(), lTowns->Get(i)->neighbors()->end());
            towns[i].loadNeighbors(towns, neighborIds);
            loadBar.progress(1. / static_cast<double>(towns.size()));
            loadBar.draw(screen.get());
            SDL_RenderPresent(screen.get());
        }
        player->loadTraveler(game->playerTraveler(), towns);
        auto lTravelers = game->aITravelers();
        auto lTI = lTravelers->begin();
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
        case SDL_MOUSEBUTTONDOWN:
            // Print r g b values at clicked coordinates
            mx = event.button.x + mapRect.x;
            my = event.button.y + mapRect.y;
            mw = mapSurface->w;
            mh = mapSurface->h;
            if (mx >= 0 and mx < mw and my >= 0 and my < mh) {
                SDL_GetRGB(getAt(mapSurface.get(), mx, my), mapSurface->format, &r, &g, &b);
                std::cout << "Clicked color: (" << int(r) << "," << int(g) << "," << int(b) << ")" << std::endl;
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
    SDL_RenderCopy(screen.get(), mapTexture.get(), nullptr, nullptr);
    for (auto &t : towns)
        t.drawRoutes(screen.get());
    for (auto &t : towns)
        t.drawDot(screen.get());
    for (auto &t : aITravelers)
        t->draw(screen.get());
    for (auto &t : towns)
        t.drawText(screen.get());
    player->draw(screen.get());
    SDL_RenderPresent(screen.get());
}

void Game::saveRoutes() {
    // Recalculate routes between towns and save new routes to database.
    LoadBar loadBar({screenRect.w / 15, screenRect.h * 7 / 15, screenRect.w * 13 / 15, screenRect.h / 15},
                    {"Finding routes..."}, {0, 255, 0, 255}, Settings::getUIForeground(), Settings::getBigBoxRadius(),
                    Settings::getBigBoxBorder(), Settings::getLoadBarFontSize(), printer);
    for (auto &t : towns) {
        t.findNeighbors(towns, mapSurface.get(), mapRect.x, mapRect.y);
        loadBar.progress(1. / static_cast<double>(towns.size()));
        loadBar.draw(screen.get());
        SDL_RenderPresent(screen.get());
    }
    for (auto &t : towns)
        t.connectRoutes();
    std::string inserts = "INSERT OR IGNORE INTO routes VALUES";
    for (auto &t : towns)
        t.saveNeighbors(inserts);
    inserts.pop_back();
    sql::DtbsPtr conn = sql::makeConnection(fs::path("1025ad.db"), SQLITE_OPEN_READWRITE);
    ;
    sql::StmtPtr comm;
    comm = sql::makeQuery(conn.get(), inserts.c_str());
    sqlite3_step(comm.get());
}

void Game::saveData() {
    // Save changes to frequencies and demand slopes to database.
    sql::DtbsPtr conn = sql::makeConnection(fs::path("1025ad.db"), SQLITE_OPEN_READWRITE);
    std::string updates = "UPDATE frequencies SET frequency = CASE";
    player->getTraveler()->getTown()->saveFrequencies(updates);
    sql::StmtPtr comm;
    comm = sql::makeQuery(conn.get(), updates.c_str());
    if (sqlite3_step(comm.get()) != SQLITE_DONE)
        std::cout << "Error updating frequencies: " << sqlite3_errmsg(conn.get()) << std::endl << updates << std::endl;
    updates = "UPDATE consumption SET demand_slope = CASE";
    player->getTraveler()->getTown()->saveDemand(updates);
    comm = sql::makeQuery(conn.get(), updates.c_str());
    if (sqlite3_step(comm.get()) != SQLITE_DONE)
        std::cout << "Error updating demand slopes: " << sqlite3_errmsg(conn.get()) << std::endl << updates << std::endl;
    if (sqlite3_close(conn.get()) != SQLITE_OK)
        std::cout << sqlite3_errmsg(conn.get()) << std::endl;
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
    auto game = Save::CreateGame(builder, sTowns, player->getTraveler()->save(builder), sAITravelers);
    builder.Finish(game);
    fs::path path("save");
    path /= player->getTraveler()->getName();
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
