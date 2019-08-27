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
    : screenRect(Settings::getScreenRect()), mapView(Settings::getMapView()), offset(Settings::getOffset()),
      scale(Settings::getScale()), window(SDL_CreateWindow("Camels", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
                                                           screenRect.w, screenRect.h, SDL_WINDOW_BORDERLESS)),
      screen(SDL_CreateRenderer(window.get(), -1, SDL_RENDERER_ACCELERATED)),
      travelersCheckCounter(Settings::travelersCheckCounter()) {
    player = std::make_unique<Player>(*this);
    player->setState(State::starting);
    std::cout << "Creating Game" << std::endl;
    if (!window.get())
        throw std::runtime_error("Window creation failed, SDL Error:" + std::string(SDL_GetError()));
    sdl::Surface icon(IMG_Load(fs::path("images/icon.png").string().c_str()));
    if (!icon.get())
        throw std::runtime_error("Failed to load icon, IMG Error: " + std::string(IMG_GetError()));
    SDL_SetWindowIcon(window.get(), icon.get());
    icon = nullptr;
    if (!screen.get())
        throw std::runtime_error("Failed to create renderer, SDL Error:" + std::string(SDL_GetError()));
    if (SDL_GetRendererInfo(screen.get(), &screenInfo) < 0)
        throw std::runtime_error("Failed to get renderer info, SDL Error:" + std::string(SDL_GetError()));
    // For debugging, pretend renderer can't handle textures over 64x64
    // screenInfo.max_texture_height = 64;
    // screenInfo.max_texture_width = 64;
    std::cout << "Loading Map" << std::endl;
    sdl::Surface mapSurface(IMG_Load(fs::path("images/map-scaled.png").string().c_str()));
    if (!mapSurface)
        throw std::runtime_error("Failed to load map surface, IMG Error:" + std::string(IMG_GetError()));
    mapRect = {0, 0, mapSurface->w, mapSurface->h};
    mapTexture.reset(SDL_CreateTexture(screen.get(), SDL_PIXELFORMAT_RGB24, SDL_TEXTUREACCESS_TARGET,
                                       mapView.w, mapView.h));
    SDL_Rect rt = {0, 0, screenInfo.max_texture_width, screenInfo.max_texture_height};
    mapTextureColumnCount = mapRect.w / rt.w + (mapRect.w % rt.w != 0);
    mapTextureRowCount = mapRect.h / rt.h + (mapRect.h % rt.h != 0);
    mapTextures.reserve(static_cast<size_t>(mapTextureColumnCount * mapTextureRowCount));
    for (; rt.y + rt.h <= mapRect.h; rt.y += rt.h) {
        // Loop from top to bottom.
        for (rt.x = 0; rt.x + rt.w <= mapRect.w; rt.x += rt.w)
            // Loop from left to right.
            mapTextures.push_back(sdl::textureFromSurfaceSection(screen.get(), mapSurface.get(), rt));
        if (rt.x < mapRect.w) {
            // Fill right side rectangles of map.
            rt.w = mapRect.w - rt.x;
            mapTextures.push_back(sdl::textureFromSurfaceSection(screen.get(), mapSurface.get(), rt));
            rt.w = screenInfo.max_texture_width;
        }
    }
    if (rt.y < mapRect.h) {
        // Fill in bottom side rectangles of map.
        rt.h = mapRect.h - rt.y;
        for (; rt.x + rt.w <= mapRect.w; rt.x += rt.w)
            mapTextures.push_back(sdl::textureFromSurfaceSection(screen.get(), mapSurface.get(), rt));
        if (rt.x < mapRect.w) {
            // Fill bottom right corner of map.
            rt.w = mapRect.w - rt.x;
            mapTextures.push_back(sdl::textureFromSurfaceSection(screen.get(), mapSurface.get(), rt));
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
    player = nullptr;
    aITravelers.clear();
    towns.clear();
    std::cout << "Freeing Map Textures" << std::endl;
    mapTextures.clear();
    std::cout << "Freeing Map Texture" << std::endl;
    mapTexture = nullptr;
    std::cout << "Freeing Good Images" << std::endl;
    goodImages.clear();
    std::cout << "Destroying Screen" << std::endl;
    screen = nullptr;
    std::cout << "Destroying Window" << std::endl;
    window = nullptr;
}

void Game::run() {
    // Run the game loop.
    while (!player->getStop()) {
        handleEvents();
        update();
        draw();
        SDL_Delay(20);
    }
}

void Game::renderMapTexture() {
    // Construct map texture for drawing from matrix of map textures.
    int left = mapView.x / screenInfo.max_texture_width, top = mapView.y / screenInfo.max_texture_height,
        right = (mapView.x + mapView.w) / screenInfo.max_texture_width,
        bottom = (mapView.y + mapView.h) / screenInfo.max_texture_height;
    SDL_Rect sR = {0, 0, 0, 0}, dR = {0, 0, 0, 0};
    if (SDL_SetRenderTarget(screen.get(), mapTexture.get()) < 0)
        throw std::runtime_error("Failed to set render target, SDL Error: " + std::string(SDL_GetError()));
    for (int c = left; c <= right; ++c) {
        dR.y = 0;
        dR.h = 0;
        for (int r = top; r <= bottom; ++r) {
            sR = {std::max(mapView.x - c * screenInfo.max_texture_width, 0),
                  std::max(mapView.y - r * screenInfo.max_texture_height, 0),
                  std::min(std::min((c + 1) * screenInfo.max_texture_width - mapView.x, screenInfo.max_texture_width),
                           mapView.w),
                  std::min(std::min((r + 1) * screenInfo.max_texture_height - mapView.y, screenInfo.max_texture_height),
                           mapView.h)};
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
    for (auto &t : towns) t.placeDot(newDrawn, offset, scale);
    for (auto &t : towns) t.placeText(newDrawn);
    for (auto &t : aITravelers) t->place(offset, scale);
    player->place(offset, scale);
}

void Game::moveView(const SDL_Point &dp) {
    // Move view around world map.
    SDL_Rect m = {mapView.x + dp.x, mapView.y + dp.y, mapRect.w, mapRect.h};
    if (m.x > 0 && m.x < m.w - mapView.w && m.y > 0 && m.y < m.h - mapView.h) {
        mapView.x = m.x;
        mapView.y = m.y;
        offset.x -= dp.x;
        offset.y -= dp.y;
        renderMapTexture();
        place();
    }
}

void Game::loadData(sqlite3 *cn) {
    // Load data from database which is needed both for new game and loading a game.
    std::cout << "Loading Data" << std::endl;
    // Load game data.
    // Load part names.
    auto quer = sql::makeQuery(cn, "SELECT name FROM parts");
    auto q = quer.get();
    for (size_t i = 0; sqlite3_step(q) != SQLITE_DONE; ++i)
        gameData.partNames[i] = std::string(reinterpret_cast<const char *>(sqlite3_column_text(q, 0)));
    // Load status names.
    quer = sql::makeQuery(cn, "SELECT name FROM statuses");
    q = quer.get();
    for (size_t i = 0; sqlite3_step(q) != SQLITE_DONE; ++i)
        gameData.statusNames[i] = std::string(reinterpret_cast<const char *>(sqlite3_column_text(q, 0)));

    // Load combat odds.
    quer = sql::makeQuery(cn,
                          "SELECT hit_odds, status_1, status_1_chance, status_2, "
                          "status_2_chance, status_3, status_3_chance FROM combat_odds");
    q = quer.get();
    for (size_t i = 0; sqlite3_step(q) != SQLITE_DONE; ++i)
        gameData.odds[i] = {sqlite3_column_double(q, 0),
                            {{{static_cast<Status>(sqlite3_column_int(q, 1)), sqlite3_column_double(q, 2)},
                              {static_cast<Status>(sqlite3_column_int(q, 3)), sqlite3_column_double(q, 4)},
                              {static_cast<Status>(sqlite3_column_int(q, 5)), sqlite3_column_double(q, 6)}}}};

    // Load town type nouns.
    quer = sql::makeQuery(cn, "SELECT noun FROM town_type_nouns");
    q = quer.get();
    for (size_t i = 0; sqlite3_step(q) != SQLITE_DONE; ++i)
        gameData.townTypeNames[i] = std::string(reinterpret_cast<const char *>(sqlite3_column_text(q, 0)));

    quer = sql::makeQuery(cn, "SELECT minimum, adjective FROM population_adjectives");
    q = quer.get();
    while (sqlite3_step(q) != SQLITE_DONE)
        gameData.populationAdjectives.emplace(std::make_pair(
            sqlite3_column_int(q, 0), std::string(reinterpret_cast<const char *>(sqlite3_column_text(q, 1)))));

    // Load from goods table.
    std::vector<Good> basicGoods;
    quer = sql::makeQuery(cn, "SELECT COUNT(*) FROM goods");
    q = quer.get();
    if (sqlite3_step(q) != SQLITE_ROW)
        throw std::runtime_error("Error counting goods: " + std::string(sqlite3_errmsg(cn)));
    basicGoods.reserve(sqlite3_column_int(q, 0));
    quer = sql::makeQuery(cn, "SELECT good_id, name, measure, shoots FROM goods");
    q = quer.get();
    while (sqlite3_step(q) != SQLITE_DONE)
        basicGoods.push_back(Good(static_cast<unsigned int>(sqlite3_column_int(q, 0)),
                                  std::string(reinterpret_cast<const char *>(sqlite3_column_text(q, 1))),
                                  std::string(reinterpret_cast<const char *>(sqlite3_column_text(q, 2))),
                                  static_cast<unsigned int>(sqlite3_column_int(q, 3))));
    // Load from materials table.
    std::vector<Good> goods;
    quer = sql::makeQuery(cn, "SELECT COUNT(*) FROM materials");
    q = quer.get();
    if (sqlite3_step(q) != SQLITE_ROW)
        throw std::runtime_error("Error counting materials: " + std::string(sqlite3_errmsg(cn)));
    goods.reserve(sqlite3_column_int(q, 0));
    quer = sql::makeQuery(cn, "SELECT good_id, material_id, perish, carry FROM materials");
    q = quer.get();
    while (sqlite3_step(q) != SQLITE_DONE)
        goods.push_back(Good(basicGoods[sqlite3_column_int(q, 0)], basicGoods[sqlite3_column_int(q, 1)],
                             goods.size(), sqlite3_column_double(q, 2), sqlite3_column_double(q, 3)));
    // Load good images.
    int m = Settings::getButtonMargin();
    int imageSize = std::min(kMaxGoodImageSize, (screenRect.h + m) / Settings::getGoodButtonRows() - m -
                                                    2 * Settings::boxSize(BoxSizeType::trade).border);
    SDL_Rect rt = {0, 0, imageSize, imageSize};
    goodImages.reserve(goods.size());
    for (auto &g : goods) {
        // Loop over goods.
        std::string name = g.getFullName();
        // Replace a space in the good's full name with a dash.
        size_t spacePos = name.find(' ');
        if (spacePos != std::string::npos) name.replace(spacePos, 1, "-");
        // Concatenate name with path.
        fs::path imagePath("images/" + name + ".png");
        if (fs::exists(imagePath)) {
            // Load image from file.
            sdl::Surface original(IMG_Load(imagePath.string().c_str()));
            // Put empty surface in vector.
            goodImages.emplace_back(SDL_CreateRGBSurface(surfaceFlags, rt.w, rt.h, bitDepth, rmask, gmask, bmask, amask));
            // Store raw pointer to empty surface.
            auto image = goodImages.back().get();
            // Scale image into empty surface.
            SDL_BlitScaled(original.get(), nullptr, image, &rt);
            // Pass image pointer to good.
            g.setImage(image);
        }
    }

    // Load combat stats.
    std::vector<CombatStat> combatStats;
    quer = sql::makeQuery(cn,
                          "SELECT good_id, material_id, stat_id, part_id, attack, type, "
                          "speed, bash_defense, cut_defense, stab_defense FROM combat_stats");
    q = quer.get();
    auto gdIt = begin(goods);
    while (sqlite3_step(q) != SQLITE_DONE) {
        std::array<unsigned int, 2> ids{static_cast<unsigned int>(sqlite3_column_int(q, 0)),
                                        static_cast<unsigned int>(sqlite3_column_int(q, 1))};
        if (gdIt->getGoodId() != ids[0] || gdIt->getMaterialId() != ids[1]) {
            gdIt->setCombatStats(combatStats);
            gdIt = std::lower_bound(gdIt, end(goods), ids, [](auto g, auto a) {
                auto gId = g.getGoodId();
                return gId < a[0] || (gId == a[0] && g.getMaterialId() < a[1]);
            });
            combatStats.clear();
        }
        combatStats.push_back({static_cast<Part>(sqlite3_column_int(q, 3)),
                               static_cast<Stat>(sqlite3_column_int(q, 2)),
                               static_cast<unsigned int>(sqlite3_column_int(q, 4)),
                               static_cast<unsigned int>(sqlite3_column_int(q, 6)),
                               static_cast<AttackType>(sqlite3_column_int(q, 5) - 1),
                               {{static_cast<unsigned int>(sqlite3_column_int(q, 7)),
                                 static_cast<unsigned int>(sqlite3_column_int(q, 8)),
                                 static_cast<unsigned int>(sqlite3_column_int(q, 9))}}});
    }
    gdIt->setCombatStats(combatStats);
    // Load businesses.
    std::vector<Business> businesses;
    quer = sql::makeQuery(
        cn,
        "SELECT business_id, modes, name, can_switch, require_coast, keep_material, city_frequency, "
        "town_frequency, fort_frequency FROM businesses");
    q = quer.get();
    unsigned int bId = 1;
    while (sqlite3_step(q) != SQLITE_DONE) {
        size_t modeCount = sqlite3_column_int(q, 1);
        businesses.push_back(
            Business(static_cast<unsigned int>(sqlite3_column_int(q, 0)), 1,
                     std::string(reinterpret_cast<const char *>(sqlite3_column_text(q, 2))),
                     static_cast<bool>(sqlite3_column_int(q, 3)), static_cast<bool>(sqlite3_column_int(q, 4)),
                     static_cast<bool>(sqlite3_column_int(q, 5)),
                     {sqlite3_column_double(q, 6), sqlite3_column_double(q, 7), sqlite3_column_double(q, 8)}));
        for (unsigned int bMd = 2; bMd <= modeCount; ++bMd)
            businesses.push_back(Business(businesses.back(), bMd));
    }
    // Load requirements.
    quer = sql::makeQuery(cn, "SELECT business_id, good_id, amount FROM requirements");
    q = quer.get();
    std::vector<Good> requirements;
    auto bIt = begin(businesses);
    while (sqlite3_step(q) != SQLITE_DONE) {
        bId = static_cast<unsigned int>(sqlite3_column_int(q, 0));
        if (bIt->getId() != bId) {
            // Business ids don't match, flush vector and increment.
            for (; bIt->getId() != bId; ++bIt)
                // Loop over modes until next business id is reached.
                bIt->setRequirements(requirements);
            requirements.clear();
        }
        auto &gd = basicGoods[sqlite3_column_int(q, 1)];
        requirements.push_back(Good(gd, sqlite3_column_double(q, 2)));
    }
    // Set requirements for last business.
    for (; bIt != end(businesses); ++bIt)
        // Loop over modes.
        bIt->setRequirements(requirements);
    // Load inputs.
    quer = sql::makeQuery(cn, "SELECT business_id, mode, good_id, amount FROM inputs");
    q = quer.get();
    std::vector<Good> inputs;
    bIt = begin(businesses);
    while (sqlite3_step(q) != SQLITE_DONE) {
        if (bIt->getId() != static_cast<unsigned int>(sqlite3_column_int(q, 0)) ||
            bIt->getMode() != static_cast<unsigned int>(sqlite3_column_int(q, 1))) {
            // Business ids or modes don't match, flush vector and increment
            bIt->setInputs(inputs);
            inputs.clear();
            ++bIt;
        }
        auto &gd = basicGoods[sqlite3_column_int(q, 2)];
        inputs.push_back(Good(gd, sqlite3_column_double(q, 3)));
    }
    // Set inputs for last business.
    bIt->setInputs(inputs);
    // Load outputs.
    quer = sql::makeQuery(cn, "SELECT business_id, mode, good_id, amount FROM outputs");
    q = quer.get();
    std::vector<Good> outputs;
    bIt = begin(businesses);
    while (sqlite3_step(q) != SQLITE_DONE) {
        if (bIt->getId() != static_cast<unsigned int>(sqlite3_column_int(q, 0)) ||
            bIt->getMode() != static_cast<unsigned int>(sqlite3_column_int(q, 1))) {
            // Business ids or modes don't match, flush vector and increment
            bIt->setOutputs(outputs);
            outputs.clear();
            ++bIt;
        }
        auto &gd = basicGoods[sqlite3_column_int(q, 2)];
        outputs.push_back(Good(gd, sqlite3_column_double(q, 3)));
    }
    // Set outputs for last business.
    bIt->setOutputs(outputs);
    // Load nations.
    quer = sql::makeQuery(cn, "SELECT COUNT(*) FROM nations");
    q = quer.get();
    if (sqlite3_step(q) != SQLITE_ROW)
        throw std::runtime_error("Error counting nations: " + std::string(sqlite3_errmsg(cn)));
    gameData.nationCount = sqlite3_column_int(q, 0);
    nations.reserve(gameData.nationCount);
    quer = sql::makeQuery(cn,
                          "SELECT nation_id, english_name, language_name, adjective, color_r, "
                          "color_g, color_b,"
                          "background_r, background_g, background_b, religion FROM nations");
    q = quer.get();
    while (sqlite3_step(q) != SQLITE_DONE)
        nations.push_back(Nation(
            static_cast<unsigned int>(sqlite3_column_int(q, 0)),
            {std::string(reinterpret_cast<const char *>(sqlite3_column_text(q, 1))),
             std::string(reinterpret_cast<const char *>(sqlite3_column_text(q, 2)))},
            std::string(reinterpret_cast<const char *>(sqlite3_column_text(q, 3))),
            {static_cast<Uint8>(sqlite3_column_int(q, 4)), static_cast<Uint8>(sqlite3_column_int(q, 5)),
             static_cast<Uint8>(sqlite3_column_int(q, 6)), 255},
            {static_cast<Uint8>(sqlite3_column_int(q, 7)), static_cast<Uint8>(sqlite3_column_int(q, 8)),
             static_cast<Uint8>(sqlite3_column_int(q, 9)), 255},
            std::string(reinterpret_cast<const char *>(sqlite3_column_text(q, 10))), goods, businesses));
    // Load traveler names into nations.
    quer = sql::makeQuery(cn, "SELECT nation_id, name FROM names");
    q = quer.get();
    size_t ntId = 1;
    std::vector<std::string> travelerNames;
    while (sqlite3_step(q) != SQLITE_DONE) {
        if (ntId != static_cast<size_t>(sqlite3_column_int(q, 0))) {
            // Nation index doesn't match, flush vector and increment.
            nations[ntId - 1].setTravelerNames(travelerNames);
            travelerNames.clear();
            ++ntId;
        }
        travelerNames.push_back(std::string(reinterpret_cast<const char *>(sqlite3_column_text(q, 1))));
    }
    // Set traveler names for last nation.
    nations[ntId - 1].setTravelerNames(travelerNames);
    // Load frequencies of businesses into nations.
    quer = sql::makeQuery(cn, "SELECT nation_id, frequency FROM frequencies");
    q = quer.get();
    ntId = 1;
    std::vector<double> frequencies;
    frequencies.reserve(businesses.size());
    while (sqlite3_step(q) != SQLITE_DONE) {
        if (ntId != static_cast<size_t>(sqlite3_column_int(q, 0))) {
            // Nation index doesn't match, flush vector and increment.
            nations[ntId - 1].setFrequencies(frequencies);
            frequencies.clear();
            ++ntId;
        }
        frequencies.push_back(sqlite3_column_double(q, 1));
    }
    // Set frequencies for last nation.
    nations[ntId - 1].setFrequencies(frequencies);
    // Load consumption information for each material of each good into nations.
    quer = sql::makeQuery(cn,
                          "SELECT nation_id, good_id, amount, demand_slope, "
                          "demand_intercept FROM consumption");
    q = quer.get();
    ntId = 1;
    std::vector<std::array<double, 3>> goodsConsumption; // good consumption data for current nation
    goodsConsumption.reserve(goods.size());
    while (sqlite3_step(q) != SQLITE_DONE) {
        if (ntId != static_cast<size_t>(sqlite3_column_int(q, 0))) {
            // Nation index doesn't match, flush vector and increment.
            nations[ntId - 1].setConsumption(goodsConsumption);
            goodsConsumption.clear();
            ++ntId;
        }
        goodsConsumption.push_back(
            {{sqlite3_column_double(q, 2), sqlite3_column_double(q, 3), sqlite3_column_double(q, 4)}});
    }
    // Flush final good consumptions vector.
    nations[ntId - 1].setConsumption(goodsConsumption);
}

void Game::loadTowns(sqlite3 *cn, LoadBar &ldBr, SDL_Texture *frzTx) {
    auto quer = sql::makeQuery(cn, "SELECT COUNT(*) FROM towns");
    auto q = quer.get();
    if (sqlite3_step(q) != SQLITE_ROW)
        throw std::runtime_error("Error counting towns: " + std::string(sqlite3_errmsg(cn)));
    // Game data holds town count for traveler properties.
    gameData.townCount = static_cast<unsigned int>(sqlite3_column_int(q, 0));
    // Store number of towns as double for progress bar purposes.
    double tC = static_cast<double>(gameData.townCount);

    quer = sql::makeQuery(cn,
                          "SELECT town_id, eng, lang, nation_id, latitude, longitude, town_type, coastal, "
                          "population FROM towns");
    q = quer.get();
    towns.reserve(gameData.townCount);
    std::cout << "Loading towns" << std::endl;
    size_t mT = Settings::getMaxTowns();
    while (sqlite3_step(q) != SQLITE_DONE && towns.size() < mT) {
        SDL_RenderCopy(screen.get(), frzTx, nullptr, nullptr);
        towns.push_back(Town(static_cast<unsigned int>(sqlite3_column_int(q, 0)),
                             {std::string(reinterpret_cast<const char *>(sqlite3_column_text(q, 1))),
                              std::string(reinterpret_cast<const char *>(sqlite3_column_text(q, 2)))},
                             &nations[static_cast<size_t>(sqlite3_column_int(q, 3) - 1)], sqlite3_column_double(q, 5),
                             sqlite3_column_double(q, 4), static_cast<TownType>(sqlite3_column_int(q, 6) - 1),
                             static_cast<bool>(sqlite3_column_int(q, 7)),
                             static_cast<unsigned long>(sqlite3_column_int(q, 8)), printer));
        // Let town run for some business cyles before game starts.
        towns.back().update(Settings::getTownHeadStart());
        ldBr.progress(1 / tC);
        ldBr.draw(screen.get());
        SDL_RenderPresent(screen.get());
    }
    place();
    // Load routes.
    quer = sql::makeQuery(cn, "SELECT COUNT(*) FROM routes");
    q = quer.get();
    if (sqlite3_step(q) != SQLITE_ROW)
        throw std::runtime_error("Error counting routes: " + std::string(sqlite3_errmsg(cn)));
    unsigned int routeCount = sqlite3_column_int(q, 0);
    routes.reserve(routeCount);
    double rC = routeCount;
    quer = sql::makeQuery(cn, "SELECT from_id, to_id FROM routes");
    q = quer.get();
    while (sqlite3_step(q) != SQLITE_DONE)
        routes.push_back(Route(&towns[sqlite3_column_int(q, 0) - 1], &towns[sqlite3_column_int(q, 1) - 1]));
    ldBr.progress(-1);
    ldBr.setText(0, "Connecting routes...");
    for (auto &rt : routes) {
        SDL_RenderCopy(screen.get(), frzTx, nullptr, nullptr);
        // Town ids don't match, flush vector and increment.
        auto &rtTwns = rt.getTowns();
        rtTwns[0]->addNeighbor(rtTwns[1]);
        rtTwns[1]->addNeighbor(rtTwns[0]);
        ldBr.progress(1 / rC);
        ldBr.draw(screen.get());
        SDL_RenderPresent(screen.get());
    }
}

const std::vector<Nation> &Game::newGame() {
    // Load data for a new game from sqlite database.
    sql::DtbsPtr conn = sql::makeConnection(fs::path("1025ad.db"), SQLITE_OPEN_READONLY);
    loadData(conn.get());

    // Load towns from sqlite database.
    LoadBar loadBar(
        Settings::boxInfo({screenRect.w / 2, screenRect.h / 2, 0, 0}, {"Loading towns..."},
                          {screenRect.w / 15, screenRect.h * 7 / 15, screenRect.w * 13 / 15, screenRect.h / 15}),
        printer);
    // Create a texture for keeping the current screen frozen behind load bar.
    sdl::Surface freezeSurface(
        SDL_CreateRGBSurface(surfaceFlags, screenRect.w, screenRect.h, bitDepth, rmask, gmask, bmask, amask));
    SDL_RenderReadPixels(screen.get(), nullptr, freezeSurface->format->format, freezeSurface->pixels,
                         freezeSurface->pitch);
    sdl::Texture freezeTexture(SDL_CreateTextureFromSurface(screen.get(), freezeSurface.get()));
    freezeSurface = nullptr;
    loadTowns(conn.get(), loadBar, freezeTexture.get());
    conn = nullptr;
    double tC = static_cast<double>(towns.size());
    loadBar.progress(-1);
    loadBar.setText(0, "Generating Travelers...");
    for (auto &t : towns) {
        SDL_RenderCopy(screen.get(), freezeTexture.get(), nullptr, nullptr);
        t.generateTravelers(gameData, aITravelers);
        loadBar.setText(0, "Generating Travelers..." + std::to_string(aITravelers.size()));
        loadBar.progress(1 / tC);
        loadBar.draw(screen.get());
        SDL_RenderPresent(screen.get());
    }
    loadBar.progress(-1);
    tC = static_cast<double>(aITravelers.size());
    loadBar.setText(0, "Starting AI...");
    for (auto &t : aITravelers) {
        SDL_RenderCopy(screen.get(), freezeTexture.get(), nullptr, nullptr);
        t->addToTown();
        t->startAI();
        loadBar.progress(1 / tC);
        loadBar.draw(screen.get());
        SDL_RenderPresent(screen.get());
    }
    return nations;
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
        size_t townCount = lTowns->size();
        towns.reserve(townCount);
        double tC = townCount;
        LoadBar loadBar(
            Settings::boxInfo({screenRect.w / 2, screenRect.h / 2, 0, 0}, {"Loading towns..."},
                              {screenRect.w / 15, screenRect.h * 7 / 15, screenRect.w * 13 / 15, screenRect.h / 15}),
            printer);
        sdl::Surface freezeSurface(SDL_CreateRGBSurface(surfaceFlags, screenRect.w, screenRect.h, bitDepth,
                                                        rmask, gmask, bmask, amask));
        SDL_RenderReadPixels(screen.get(), nullptr, freezeSurface->format->format, freezeSurface->pixels,
                             freezeSurface->pitch);
        sdl::Texture freezeTexture(SDL_CreateTextureFromSurface(screen.get(), freezeSurface.get()));
        freezeSurface = nullptr;
        std::transform(lTowns->begin(), lTowns->end(), std::back_inserter(towns),
                       [this, &freezeTexture, &loadBar, tC](auto ldTn) {
                           SDL_RenderCopy(screen.get(), freezeTexture.get(), nullptr, nullptr);
                           loadBar.progress(1 / tC);
                           loadBar.draw(screen.get());
                           SDL_RenderPresent(screen.get());
                           return Town(ldTn, nations, printer);
                       });
        auto lRoutes = game->routes();
        size_t routeCount = lRoutes->size();
        routes.reserve(routeCount);
        double rC = routeCount;
        loadBar.progress(-1);
        loadBar.setText(0, "Connecting routes...");
        for (auto lRI = lRoutes->begin(); lRI != lRoutes->end(); ++lRI) {
            SDL_RenderCopy(screen.get(), freezeTexture.get(), nullptr, nullptr);
            routes.push_back(Route(*lRI, towns));
            auto &rtTns = routes.back().getTowns();
            rtTns[0]->addNeighbor(rtTns[1]);
            rtTns[1]->addNeighbor(rtTns[0]);
            loadBar.progress(1 / rC);
            loadBar.draw(screen.get());
            SDL_RenderPresent(screen.get());
        }
        player->loadTraveler(game->playerTraveler(), nations, towns, gameData);
        auto lTravelers = game->aITravelers();
        std::transform(lTravelers->begin() + 1, lTravelers->end(), std::back_inserter(aITravelers), [this](auto ldTvl) {
            return std::make_unique<Traveler>(ldTvl, nations, towns, gameData);
        });
        for (auto &t : aITravelers) t->startAI();
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
            mx = event.button.x + mapView.x;
            my = event.button.y + mapView.y;
            mw = mapView.w;
            mh = mapView.h;
            /*if (mx >= 0 && mx < mw && my >= 0 && my < mh) {
                SDL_GetRGB(getAt(mapSurface.get(), mx, my), mapSurface->format, &r, &g, &b);
                std::cout << "Clicked Color: (" << int(r) << ", " << int(g) << ", " << int(b) << ")" << std::endl;
            }*/
            break;
        }
        player->handleEvent(event);
    }
}

void Game::update() {
    currentTime = SDL_GetTicks();
    unsigned int elapsed = currentTime - lastTime;
    lastTime = currentTime;
    if (!(player->getPause())) {
        for (auto &t : towns) t.update(elapsed);
        for (auto &t : aITravelers) {
            t->update(elapsed);
            t->place(offset, scale);
        }
        if (!aITravelers.empty()) {
            travelersCheckCounter += elapsed;
            if (travelersCheckCounter > 0) {
                aITravelers.erase(
                    std::remove_if(begin(aITravelers), end(aITravelers),
                                   [](const std::unique_ptr<Traveler> &tvl) { return tvl->getDead(); }),
                    end(aITravelers));
                travelersCheckCounter -= Settings::getTravelersCheckTime();
            }
        }
    }
    player->update(elapsed);
    player->place(offset, scale);
}

void Game::draw() {
    // Draw UI and game elements.
    SDL_RenderCopy(screen.get(), mapTexture.get(), nullptr, nullptr);
    for (auto &rt : routes) rt.draw(screen.get());
    for (auto &tn : towns) tn.draw(screen.get());
    for (auto &tv : aITravelers) tv->draw(screen.get());
    for (auto &tn : towns) tn.getBox()->draw(screen.get());
    player->draw(screen.get());
    SDL_RenderPresent(screen.get());
}

void Game::generateRoutes() {
    // Recalculate routes between towns and save new routes to database.
    LoadBar loadBar(
        Settings::boxInfo({screenRect.w / 2, screenRect.h / 2, 0, 0}, {"Finding routes..."},
                          {screenRect.w / 15, screenRect.h * 7 / 15, screenRect.w * 13 / 15, screenRect.h / 15}),
        printer);
    /*
    // Have each town find its nearest neighbors.
    for (auto &t : towns) {
        t.findNeighbors(towns, mapSurface.get(), mapView.x, mapView.y);
        loadBar.progress(1 / static_cast<double>(towns.size()));
        loadBar.draw(screen.get());
        SDL_RenderPresent(screen.get());
    }*/
    // Link every route both ways.
    for (auto &t : towns) t.connectRoutes();
    // Re-fill routes.
    routes.clear();
    for (auto &t : towns)
        for (auto &n : t.getNeighbors())
            if (t.getId() < n->getId())
                // Only fill routes one way.
                routes.push_back(Route(&t, n));
}

void Game::saveData() {
    // Save changes to frequencies and demand slopes to database.
    sql::DtbsPtr conn = sql::makeConnection(fs::path("1025ad.db"), SQLITE_OPEN_READWRITE);
    std::string updates = "UPDATE frequencies SET frequency = CASE";
    player->getTraveler()->town()->saveFrequencies(updates);
    sql::StmtPtr comm;
    comm = sql::makeQuery(conn.get(), updates.c_str());
    if (sqlite3_step(comm.get()) != SQLITE_DONE)
        throw std::runtime_error(updates + " error: " + std::string(sqlite3_errmsg(conn.get())));
    updates = "UPDATE consumption SET demand_slope = CASE";
    player->getTraveler()->town()->saveDemand(updates);
    comm = sql::makeQuery(conn.get(), updates.c_str());
    if (sqlite3_step(comm.get()) != SQLITE_DONE)
        throw std::runtime_error(updates + " error: " + std::string(sqlite3_errmsg(conn.get())));
    updates = "INSERT OR IGNORE INTO routes VALUES";
    for (auto &r : routes) r.saveData(updates);
    updates.pop_back();
    comm = sql::makeQuery(conn.get(), updates.c_str());
    if (sqlite3_step(comm.get()) != SQLITE_DONE)
        throw std::runtime_error(updates + " error: " + std::string(sqlite3_errmsg(conn.get())));
    comm = nullptr;
    conn = nullptr;
}

void Game::saveGame() {
    // Save the game.
    if (!player->hasTraveler()) std::cout << "Tried to save game with no player traveler" << std::endl;
    flatbuffers::FlatBufferBuilder builder(1024);
    auto sTowns = builder.CreateVector<flatbuffers::Offset<Save::Town>>(
        towns.size(), [this, &builder](size_t i) { return towns[i].save(builder); });
    auto sRoutes = builder.CreateVector<flatbuffers::Offset<Save::Route>>(
        routes.size(), [this, &builder](size_t i) { return routes[i].save(builder); });
    auto sAITravelers = builder.CreateVector<flatbuffers::Offset<Save::Traveler>>(
        aITravelers.size(), [this, &builder](size_t i) { return aITravelers[i]->save(builder); });
    auto game = Save::CreateGame(builder, sTowns, sRoutes, player->getTraveler()->save(builder), sAITravelers);
    builder.Finish(game);
    fs::path path("save");
    path /= player->getTraveler()->getName();
    path.replace_extension("sav");
    std::ofstream file(path.string(), std::ofstream::binary);
    if (file.is_open())
        file.write(reinterpret_cast<const char *>(builder.GetBufferPointer()), builder.GetSize());
}

std::vector<TextBox *> Game::getTownBoxes() const {
    std::vector<TextBox *> townBoxes;
    townBoxes.reserve(towns.size());
    for (auto &tn : towns) townBoxes.push_back(tn.getBox());
    return townBoxes;
}

std::unique_ptr<Traveler> Game::createPlayerTraveler(size_t nId, std::string n) {
    if (n.empty()) n = nations[nId].randomName();
    // Create traveler object for player
    auto traveler = std::make_unique<Traveler>(n, &towns[nId - 1], gameData);
    traveler->addToTown();
    traveler->place(offset, scale);
    for (auto &sG : Settings::getPlayerStartingGoods()) traveler->create(sG.first, sG.second);
    return traveler;
}

void Game::pickTown(Traveler *t, size_t tIdx) { t->pickTown(&towns[tIdx]); }
