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

#include "game.h"

Game::Game() : scrollX(0), scrollY(0) {
    Settings::load("settings.ini");
    loadDisplayVariables();
    window = SDL_CreateWindow("Camels", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, screenRect.w, screenRect.h,
                              SDL_WINDOW_BORDERLESS);
    SDL_Surface *icon = SDL_LoadBMP("icon.bmp");
    SDL_SetWindowIcon(window, icon);
    SDL_FreeSurface(icon);
    screen = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
    mapSurface = IMG_Load("map-scaled.png");
    mapTexture = SDL_CreateTextureFromSurface(screen, mapSurface);
    setState(starting);
    /*SDL_CreateRGBSurfaceWithFormat(
    0, 14220, 6640, screen->format->BitsPerPixel, screen->format->format);
    SDL_BlitScaled(mapOriginal, NULL, mapImage, NULL);*/
    SDL_ShowWindow(window);
    SDL_RaiseWindow(window);
    while (not stop) {
        handleEvents();
        update();
        draw();
        SDL_Delay(20);
    }
    Settings::save("settings.ini");
}

Game::~Game() {
    if (mapSurface)
        SDL_FreeSurface(mapSurface);
    mapSurface = nullptr;
    if (mapTexture)
        SDL_DestroyTexture(mapTexture);
    mapTexture = nullptr;
    SDL_DestroyRenderer(screen);
    SDL_DestroyWindow(window);
}

void Game::loadDisplayVariables() {
    // Load screen height and width and starting offsets and scale from settings.
    mapRect = Settings::getMapRect();
    screenRect = {0, 0, mapRect.w, mapRect.h};
    scrollSpeed = Settings::getScroll();
    offsetX = Settings::getOffsetX();
    offsetY = Settings::getOffsetY();
    scale = Settings::getScale();
}

void Game::changeOffsets(int dx, int dy) {
    // Move view around world map.
    int mx = mapRect.x + dx;
    int my = mapRect.y + dy;
    int mw = mapSurface->w;
    int mh = mapSurface->h;
    if (mx > 0 and mx < mw - mapRect.w and my > 0 and my < mh - mapRect.h) {
        mapRect.x = mx;
        mapRect.y = my;
        offsetX -= dx;
        offsetY -= dy;
        std::vector<SDL_Rect> newDrawn;
        newDrawn.reserve(towns.size());
        for (auto &t : towns)
            t.placeDot(newDrawn, offsetX, offsetY, scale);
        for (auto &t : towns)
            t.placeText(newDrawn);
        for (auto &t : travelers)
            t->place(offsetX, offsetY, scale);
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
    focusBox = -1;
    boxes.clear();
    sqlite3 *conn;
    if (sqlite3_open("1025ad.db", &conn) != SQLITE_OK)
        std::cout << "Error opening sqlite database:" << sqlite3_errmsg(conn) << std::endl;
    loadNations(conn);
    // Load towns from sqlite database.
    LoadBar loadBar({screenRect.w / 15, screenRect.h * 7 / 15, screenRect.w * 13 / 15, screenRect.h / 15},
                    {"Loading towns..."}, Settings::getLoadBarColor(), Settings::getUIForeground(),
                    Settings::getBigBoxBorder(), Settings::getBigBoxRadius(), Settings::getLoadBarDivisor());
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
    size_t tC = static_cast<size_t>(sqlite3_column_int(quer, 0)); // town count
    sqlite3_finalize(quer);
    sqlite3_prepare_v2(conn, "SELECT * FROM towns", -1, &quer, nullptr);
    towns.reserve(tC);
    while (sqlite3_step(quer) != SQLITE_DONE and towns.size() < kMaxTowns) {
        towns.push_back(Town(quer, nations, businesses, frequencyFactors, screenRect.h / Settings::getTownFontDivisor()));
        loadBar.progress(1. / static_cast<double>(tC));
        loadBar.draw(screen);
        SDL_RenderPresent(screen);
    }
    sqlite3_finalize(quer);
    changeOffsets(0, 0);
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
        t.loadNeighbors(towns, routes[t.getId() - 1]);
        t.update(Settings::getBusinessRunTime());
        loadBar.progress(1. / static_cast<double>(tC));
        loadBar.draw(screen);
        SDL_RenderPresent(screen);
    }
    loadBar.progress(-1);
    loadBar.setText(0, "Generating Travelers...");
    for (auto &t : towns) {
        t.generateTravelers(&gameData, travelers);
        loadBar.setText(0, "Generating Travelers..." + std::to_string(travelers.size()));
        loadBar.progress(1. / static_cast<double>(tC));
        loadBar.draw(screen);
        SDL_RenderPresent(screen);
    }
    loadBar.progress(-1);
    tC = travelers.size();
    loadBar.setText(0, "Starting AI...");
    for (auto &t : travelers) {
        t->startAI();
        t->addToTown();
        loadBar.progress(1. / static_cast<double>(tC));
        loadBar.draw(screen);
        SDL_RenderPresent(screen);
    }
    SDL_Rect rt = {screenRect.w / 2, screenRect.h / 7, 0, 0};
    // Create textbox for entering name.
    std::vector<std::string> tx = {"Name", ""};
    boxes.push_back(std::make_unique<TextBox>(rt, tx, Settings::getUIForeground(), Settings::getUIBackground(),
                                              Settings::getBigBoxBorder(), Settings::getBigBoxRadius(),
                                              screenRect.h / Settings::getBigBoxFontDivisor()));
    boxes.back()->toggleLock();
    for (auto &n : nations) {
        // Create a button for each nation to start in that nation.
        rt = {screenRect.w * (static_cast<int>(n.getId() - 1) % 3 * 2 + 1) / 6,
              screenRect.h * (static_cast<int>(n.getId() - 1) / 3 + 2) / 7, 0, 0};
        boxes.push_back(std::make_unique<MenuButton>(
            rt, n.getNames(), n.getForeground(), n.getBackground(), n.getId(), true, 3, Settings::getBigBoxRadius(),
            screenRect.h / Settings::getBigBoxFontDivisor(), [this, n] {
                focusBox = -1;
                unsigned int nId = n.getId(); // nation id
                std::string name = boxes.front()->getText(1);
                if (name.empty())
                    name = n.randomName();
                // Add player at end of travelers.
                travelers.push_back(std::make_shared<Traveler>(name, &towns[nId - 1], &gameData));
                // Swap player to front.
                std::swap(travelers.front(), travelers.back());
                player = travelers.front().get();
                player->addToTown();
                showPlayer = true;
                changeOffsets(0, 0);
                setState(traveling);
            }));
    }
}

void Game::load(const fs::path &p) {
    // Load a saved game from the given path.
    std::ifstream file(p.string(), std::ifstream::binary);
    if (file.is_open()) {
        sqlite3 *conn;
        if (sqlite3_open("1025ad.db", &conn) != SQLITE_OK)
            std::cout << sqlite3_errmsg(conn) << std::endl;
        nations.clear();
        loadNations(conn);
        sqlite3_close(conn);
        travelers.clear();
        towns.clear();
        storedBoxes.clear();
        boxes.clear();
        focusBox = -1;
        focusTown = -1;
        pause = false;
        file.seekg(0, file.end);
        std::streamsize length = file.tellg();
        file.seekg(0, file.beg);
        char *buffer = new char[static_cast<size_t>(length)];
        file.read(buffer, length);
        auto game = Save::GetGame(buffer);
        auto lTowns = game->towns();
        LoadBar loadBar({screenRect.w / 15, screenRect.h * 7 / 15, screenRect.w * 13 / 15, screenRect.h / 15},
                        {"Loading towns..."}, Settings::getLoadBarColor(), Settings::getUIForeground(),
                        Settings::getBigBoxBorder(), Settings::getBigBoxRadius(), Settings::getLoadBarDivisor());
        towns.reserve(lTowns->size());
        for (auto lTI = lTowns->begin(); lTI != lTowns->end(); ++lTI) {
            towns.push_back(Town(*lTI, nations, screenRect.h / Settings::getTownFontDivisor()));
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
        auto lTravelers = game->travelers();
        for (auto lTI = lTravelers->begin(); lTI != lTravelers->end(); ++lTI)
            travelers.push_back(std::make_shared<Traveler>(*lTI, towns, nations, &gameData));
        player = travelers.front().get();
        for (auto tI = travelers.begin() + 1; tI != travelers.end(); ++tI)
            (*tI)->startAI();
        showPlayer = true;
        changeOffsets(0, 0);
        setState(traveling);
    }
}

void Game::prepFocus(Focusable::FocusGroup g, int &i, int &s, std::vector<Focusable *> &fcbls) {
    std::vector<Town *> neighbors;
    switch (g) {
    case Focusable::box:
        i = focusBox;
        s = static_cast<int>(boxes.size());
        for (auto &b : boxes)
            fcbls.push_back(b.get());
        break;
    case Focusable::neighbor:
        neighbors = player->getTown()->getNeighbors();
        i = static_cast<int>(
            std::find_if(neighbors.begin(), neighbors.end(),
                         [this](Town *t) { return t->getId() == static_cast<unsigned int>(focusTown + 1); }) -
            neighbors.begin());
        s = static_cast<int>(neighbors.size());
        if (i == s) {
            if (focusTown > -1 and size_t(focusTown) < towns.size())
                towns[static_cast<size_t>(focusTown)].unFocus();
            i = -1;
        }
        fcbls = std::vector<Focusable *>(neighbors.begin(), neighbors.end());
        break;
    case Focusable::town:
        i = focusTown;
        s = static_cast<int>(towns.size());
        for (auto &t : towns)
            fcbls.push_back(&t);
        break;
    }
}

void Game::finishFocus(int f, Focusable::FocusGroup g, const std::vector<Focusable *> &fcbls) {
    switch (g) {
    case Focusable::box:
        focusBox = f;
        break;
    case Focusable::neighbor:
        if (f > -1)
            focusTown = static_cast<int>(fcbls[static_cast<size_t>(f)]->getId() - 1);
        else
            focusTown = -1;
        break;
    case Focusable::town:
        focusTown = f;
        break;
    }
}

void Game::focus(int f, Focusable::FocusGroup g) {
    // Focus item f from group g.
    int i, s;
    std::vector<Focusable *> fcbls;
    prepFocus(g, i, s, fcbls);
    if (i > -1 and i < s)
        fcbls[static_cast<size_t>(i)]->unFocus();
    if (f > -1 and f < s)
        fcbls[static_cast<size_t>(f)]->focus();
    finishFocus(f, g, fcbls);
}

void Game::focusPrev(Focusable::FocusGroup g) {
    // Focus previous item from group g.
    int i, s;
    std::vector<Focusable *> fcbls;
    prepFocus(g, i, s, fcbls);
    if (i == -1) {
        i = s;
        while (i--)
            if (fcbls[static_cast<size_t>(i)]->getCanFocus())
                break;
    } else {
        fcbls[static_cast<size_t>(i)]->unFocus();
        int j = i;
        while (--i != j)
            if (i < 0)
                i = s;
            else if (fcbls[static_cast<size_t>(i)]->getCanFocus())
                break;
    }
    if (i > -1) {
        fcbls[static_cast<size_t>(i)]->focus();
        finishFocus(i, g, fcbls);
    }
}

void Game::focusNext(Focusable::FocusGroup g) {
    // Focus next item from group g.
    int i, s;
    std::vector<Focusable *> fcbls;
    prepFocus(g, i, s, fcbls);
    if (i == -1) {
        while (++i < s)
            if (fcbls[static_cast<size_t>(i)]->getCanFocus())
                break;
    } else {
        fcbls[static_cast<size_t>(i)]->unFocus();
        int j = i;
        while (++i != j)
            if (i >= s)
                i = -1;
            else if (fcbls[static_cast<size_t>(i)]->getCanFocus())
                break;
    }
    if (i == s)
        i = -1;
    else {
        fcbls[static_cast<size_t>(i)]->focus();
        finishFocus(i, g, fcbls);
    }
}

void Game::setState(UIState s) {
    // Change the UI state to s.
    boxes.clear();
    focusBox = -1;
    switch (s) {
    case starting:
        createStartButtons();
        break;
    case quitting:
        createQuitButtons();
        break;
    case loading:
        createLoadButtons();
        break;
    case traveling:
        createTravelButtons();
        break;
    case logging:
        createLogBox();
        break;
    case trading:
        createTradeButtons();
        break;
    case equipping:
        createEquipButtons();
        break;
    case managing:
        createManageButtons();
        break;
    case attacking:
        createAttackButton();
        break;
    case fighting:
        createFightBoxes();
        break;
    case looting:
        createLootButtons();
        break;
    case dying:
        createDyingBoxes();
        break;
    }
    state = s;
}

void Game::toggleState(UIState s) {
    // Set UI state to s if not already s, otherwise set to traveling.
    if (s == quitting and state == loading)
        s = loading;
    if (s == quitting or s == loading) {
        // Store or restore UI state.
        focus(-1, Focusable::box);
        std::swap(boxes, storedBoxes);
        std::swap(pause, storedPause);
        std::swap(state, storedState);
        if (storedState == s)
            // Stored boxes have been restored, no need to create new boxes.
            return;
    }
    if (state == s and not(s == quitting or s == loading))
        setState(traveling);
    else if (not(state == fighting or state == dying) or s == quitting)
        setState(s);
}

void Game::updateUI() {
    // Update current UI to reflect current game state.
    if (not player)
        return;
    auto target = player->getTarget().lock();
    switch (state) {
    case traveling:
        if (target) {
            pause = true;
            setState(fighting);
        }
        break;
    case trading:
        player->updateTradeButtons(boxes);
        break;
    case fighting:
        if (not player->alive()) {
            setState(dying);
            break;
        } else if (not target) {
            setState(logging);
            break;
        } else if (player->fightWon()) {
            setState(looting);
            break;
        }
        player->updateFightBoxes(boxes);
        break;
    default:
        break;
    }
}

void Game::createStartButtons() {
    SDL_Rect rt = {screenRect.w / 2, screenRect.h / 15, 0, 0};
    std::vector<std::string> tx = {"Camels and Silk"};
    boxes.push_back(std::make_unique<TextBox>(rt, tx, Settings::getUIForeground(), Settings::getUIBackground(),
                                              Settings::getBigBoxBorder(), Settings::getBigBoxRadius(),
                                              screenRect.h / Settings::getBigBoxFontDivisor()));
    rt = {screenRect.w / 7, screenRect.h / 3, 0, 0};
    tx = {"(N)ew Game"};
    boxes.push_back(std::make_unique<MenuButton>(rt, tx, Settings::getUIForeground(), Settings::getUIBackground(),
                                                 Settings::getBigBoxBorder(), Settings::getBigBoxRadius(),
                                                 screenRect.h / Settings::getBigBoxFontDivisor(), [this] { newGame(); }));
    boxes.back()->setKey(SDLK_n);
    rt = {screenRect.w / 7, screenRect.h * 2 / 3, 0, 0};
    tx = {"(L)oad Game"};
    boxes.push_back(std::make_unique<MenuButton>(rt, tx, Settings::getUIForeground(), Settings::getUIBackground(),
                                                 Settings::getBigBoxBorder(), Settings::getBigBoxRadius(),
                                                 screenRect.h / Settings::getBigBoxFontDivisor(), [this] {
                                                     boxes.back()->setClicked(false);
                                                     toggleState(loading);
                                                 }));
    boxes.back()->setKey(SDLK_l);
}

void Game::createQuitButtons() {
    // Bring up quit menu.
    SDL_Rect r = {screenRect.w / 2, screenRect.h / 4, 0, 0};
    std::vector<std::string> tx = {"Continue"};
    boxes.push_back(std::make_unique<MenuButton>(
        r, tx, Settings::getUIForeground(), Settings::getUIBackground(), Settings::getBigBoxBorder(),
        Settings::getBigBoxRadius(), screenRect.h / Settings::getBigBoxFontDivisor(), [this] { toggleState(quitting); }));
    r = {screenRect.w / 2, screenRect.h * 3 / 4, 0, 0};
    std::function<void()> f;
    if (not travelers.empty()) {
        tx = {"Save and Quit"};
        f = [this] {
            save();
            stop = true;
        };
    } else {
        tx = {"Quit"};
        f = [this] { stop = true; };
    }
    boxes.push_back(std::make_unique<MenuButton>(r, tx, Settings::getUIForeground(), Settings::getUIBackground(),
                                                 Settings::getBigBoxBorder(), Settings::getBigBoxRadius(),
                                                 screenRect.h / Settings::getBigBoxFontDivisor(), f));
    focus(1, Focusable::box);
}

void Game::createLoadButtons() {
    SDL_Rect rt = {screenRect.w / 7, screenRect.h / 7, 0, 0};
    std::vector<std::string> tx = {"(B)ack"};
    boxes.push_back(std::make_unique<MenuButton>(
        rt, tx, Settings::getUIForeground(), Settings::getUIBackground(), Settings::getBigBoxBorder(),
        Settings::getBigBoxRadius(), screenRect.h / Settings::getBigBoxFontDivisor(), [this] { toggleState(loading); }));
    boxes.back()->setKey(SDLK_b);
    fs::path path = "save";
    std::vector<std::string> saves;
    for (auto &f : fs::directory_iterator(path))
        saves.push_back(f.path().stem().string());
    rt = {screenRect.w / 2, screenRect.h / 15, 0, 0};
    tx = {"Load"};
    boxes.push_back(std::make_unique<TextBox>(rt, tx, Settings::getUIForeground(), Settings::getUIBackground(),
                                              Settings::getBigBoxBorder(), Settings::getBigBoxRadius(),
                                              screenRect.h / Settings::getBigBoxFontDivisor()));
    rt = {screenRect.w / 5, screenRect.h / 7, screenRect.w * 3 / 5, screenRect.h * 5 / 7};
    boxes.push_back(std::make_unique<SelectButton>(
        rt, saves, Settings::getUIForeground(), Settings::getUIBackground(), Settings::getUIHighlight(),
        Settings::getBigBoxBorder(), Settings::getBigBoxRadius(), screenRect.h / Settings::getBigBoxFontDivisor(),
        [this, path] { load((path / boxes.back()->getItem()).replace_extension("sav")); }));
    focus(2, Focusable::box);
}

void Game::createTravelButtons() {
    SDL_Rect rt = {screenRect.w / 6, screenRect.h * 14 / 15, 0, 0};
    std::vector<std::string> tx = {"(T)rade"};
    boxes.push_back(std::make_unique<MenuButton>(
        rt, tx, Settings::getUIForeground(), Settings::getUIBackground(), Settings::getSmallBoxBorder(),
        Settings::getSmallBoxRadius(), screenRect.h / Settings::getSmallBoxFontDivisor(), [this] { setState(trading); }));
    boxes.back()->setKey(SDLK_t);
    rt.x += screenRect.w / 6;
    tx = {"(G)o"};
    boxes.push_back(std::make_unique<MenuButton>(rt, tx, Settings::getUIForeground(), Settings::getUIBackground(),
                                                 Settings::getSmallBoxBorder(), Settings::getSmallBoxRadius(),
                                                 screenRect.h / Settings::getSmallBoxFontDivisor(), [this] {
                                                     if (focusTown > -1)
                                                         player->pickTown(&towns[static_cast<size_t>(focusTown)]);
                                                     showPlayer = true;
                                                     boxes[1]->setClicked(false);
                                                 }));
    boxes.back()->setKey(SDLK_g);
    rt.x += screenRect.w / 6;
    tx = {"(E)quip"};
    boxes.push_back(std::make_unique<MenuButton>(
        rt, tx, Settings::getUIForeground(), Settings::getUIBackground(), Settings::getSmallBoxBorder(),
        Settings::getSmallBoxRadius(), screenRect.h / Settings::getSmallBoxFontDivisor(), [this] { setState(equipping); }));
    boxes.back()->setKey(SDLK_e);
    rt.x += screenRect.w / 6;
    tx = {"(F)ight"};
    boxes.push_back(std::make_unique<MenuButton>(
        rt, tx, Settings::getUIForeground(), Settings::getUIBackground(), Settings::getSmallBoxBorder(),
        Settings::getSmallBoxRadius(), screenRect.h / Settings::getSmallBoxFontDivisor(), [this] { setState(attacking); }));
    boxes.back()->setKey(SDLK_f);
    rt.x += screenRect.w / 6;
    tx = {"View (L)og"};
    boxes.push_back(std::make_unique<MenuButton>(
        rt, tx, Settings::getUIForeground(), Settings::getUIBackground(), Settings::getSmallBoxBorder(),
        Settings::getSmallBoxRadius(), screenRect.h / Settings::getSmallBoxFontDivisor(), [this] { setState(logging); }));
    boxes.back()->setKey(SDLK_l);
    pause = false;
    focus(0, Focusable::box);
}

void Game::createLogBox() {
    SDL_Rect r = {screenRect.w * 5 / 6, screenRect.h * 14 / 15, 0, 0};
    std::vector<std::string> tx = {"Close (L)og"};
    boxes.push_back(std::make_unique<MenuButton>(
        r, tx, Settings::getUIForeground(), Settings::getUIBackground(), Settings::getSmallBoxBorder(),
        Settings::getSmallBoxRadius(), screenRect.h / Settings::getSmallBoxFontDivisor(), [this] { setState(traveling); }));
    boxes.back()->setKey(SDLK_l);
    // Create log box.
    r = {screenRect.w / 15, screenRect.h * 2 / 15, screenRect.w * 13 / 15, screenRect.h * 11 / 15};
    boxes.push_back(std::make_unique<ScrollBox>(r, player->getLogText(), player->getNation()->getForeground(),
                                                player->getNation()->getBackground(), player->getNation()->getHighlight(),
                                                Settings::getBigBoxBorder(), Settings::getBigBoxRadius(),
                                                screenRect.h / Settings::getFightFontDivisor()));
    boxes.back()->setHighlightLine(-1);
}

void Game::createTradeButtons() {
    SDL_Rect rt = {screenRect.w / 6, screenRect.h * 14 / 15, 0, 0};
    std::vector<std::string> tx = {"Stop (T)rading"};
    boxes.push_back(std::make_unique<MenuButton>(
        rt, tx, Settings::getUIForeground(), Settings::getUIBackground(), Settings::getSmallBoxBorder(),
        Settings::getSmallBoxRadius(), screenRect.h / Settings::getSmallBoxFontDivisor(), [this] { setState(traveling); }));
    boxes.back()->setKey(SDLK_t);
    rt.x += screenRect.w / 6;
    tx = {player->tradePortionString()};
    boxes.push_back(std::make_unique<TextBox>(rt, tx, Settings::getUIForeground(), Settings::getUIBackground(),
                                              Settings::getSmallBoxBorder(), Settings::getSmallBoxRadius(),
                                              screenRect.h / Settings::getSmallBoxFontDivisor()));
    boxes.back()->toggleLock();
    rt.x -= screenRect.w / 6;
    rt.y += screenRect.h / 31;
    tx = {"(C)omplete Trade"};
    boxes.push_back(std::make_unique<MenuButton>(rt, tx, Settings::getUIForeground(), Settings::getUIBackground(),
                                                 Settings::getSmallBoxBorder(), Settings::getSmallBoxRadius(),
                                                 screenRect.h / Settings::getSmallBoxFontDivisor(), [this] {
                                                     player->makeTrade();
                                                     setState(trading);
                                                 }));
    boxes.back()->setKey(SDLK_c);
    rt.x += screenRect.w / 6;
    tx = {"(S)et Portion"};
    boxes.push_back(std::make_unique<MenuButton>(rt, tx, Settings::getUIForeground(), Settings::getUIBackground(),
                                                 Settings::getSmallBoxBorder(), Settings::getSmallBoxRadius(),
                                                 screenRect.h / Settings::getSmallBoxFontDivisor(), [this] {
                                                     double p;
                                                     std::stringstream(boxes[kTradePortionIndex]->getText().back()) >> p;
                                                     if (p > 1.0) {
                                                         boxes[kTradePortionIndex]->setText({"1.0"});
                                                         p = 1.0;
                                                     } else if (p < 0.0) {
                                                         boxes[kTradePortionIndex]->setText({"0.0"});
                                                         p = 0.0;
                                                     }
                                                     player->setTradePortion(p);
                                                     boxes[kTradePortionIndex]->setClicked(false);
                                                 }));
    boxes.back()->setKey(SDLK_s);
    // function to call when boxes are clicked
    auto f = [this] { player->updateTradeButtons(boxes); };
    // Create the offer and request buttons for the player.
    int left = screenRect.w / Settings::getGoodButtonXDivisor(), right = screenRect.w / 2;
    int top = screenRect.h / Settings::getGoodButtonXDivisor();
    int dx = screenRect.w * 31 / Settings::getGoodButtonXDivisor(),
        dy = screenRect.h * 31 / Settings::getGoodButtonYDivisor();
    rt = {left, top, screenRect.w * 29 / Settings::getGoodButtonXDivisor(),
          screenRect.h * 29 / Settings::getGoodButtonYDivisor()};
    for (auto &g : player->getGoods()) {
        for (auto &m : g.getMaterials())
            if ((m.getAmount() >= 0.01 and g.getSplit()) or (m.getAmount() >= 1)) {
                boxes.push_back(m.button(true, g.getId(), g.getName(), g.getSplit(), rt,
                                         player->getNation()->getForeground(), player->getNation()->getBackground(),
                                         screenRect.h / Settings::getTradeFontDivisor(), f));
                rt.x += dx;
                if (rt.x + rt.w >= right) {
                    rt.x = left;
                    rt.y += dy;
                }
            }
    }
    left = screenRect.w / 2 + screenRect.w / Settings::getGoodButtonXDivisor();
    right = screenRect.w - screenRect.w / Settings::getGoodButtonXDivisor();
    rt.x = left;
    rt.y = top;
    player->setRequestButtonIndex(static_cast<std::vector<std::unique_ptr<TextBox>>::difference_type>(boxes.size()));
    for (auto &g : player->getTown()->getGoods()) {
        for (auto &m : g.getMaterials())
            if ((m.getAmount() >= 0.00 and g.getSplit()) or (m.getAmount() >= 0)) {
                boxes.push_back(m.button(
                    true, g.getId(), g.getName(), g.getSplit(), rt, player->getTown()->getNation()->getForeground(),
                    player->getTown()->getNation()->getBackground(), screenRect.h / Settings::getTradeFontDivisor(), f));
                rt.x += dx;
                if (rt.x + rt.w >= right) {
                    rt.x = left;
                    rt.y += dy;
                }
            }
    }
    focus(0, Focusable::box);
}

void Game::createEquipButtons() {
    // Create buttons for equipping and unequipping items.
    SDL_Rect rt = {screenRect.w / 2, screenRect.h * 14 / 15, 0, 0};
    std::vector<std::string> tx = {"Stop (E)quipping"};
    boxes.push_back(std::make_unique<MenuButton>(
        rt, tx, Settings::getUIForeground(), Settings::getUIBackground(), Settings::getSmallBoxBorder(),
        Settings::getSmallBoxRadius(), screenRect.h / Settings::getSmallBoxFontDivisor(), [this] { setState(traveling); }));
    boxes.back()->setKey(SDLK_e);
    // Create array of vectors corresponding to each part that can hold equipment.
    std::array<std::vector<Good>, 6> equippable;
    for (auto &g : player->getGoods())
        for (auto &m : g.getMaterials()) {
            auto &ss = m.getCombatStats();
            if (not ss.empty() and m.getAmount() >= 1) {
                size_t i = ss.front().partId;
                Good e(g.getId(), g.getName(), 1);
                Material eM(m.getId(), m.getName(), 1);
                eM.setCombatStats(ss);
                e.addMaterial(eM);
                equippable[i].push_back(e);
            }
        }
    // Create buttons for equipping equippables
    int left = screenRect.w / Settings::getGoodButtonXDivisor(), top = screenRect.h / Settings::getGoodButtonYDivisor();
    int dx = screenRect.w * 36 / Settings::getGoodButtonXDivisor(),
        dy = screenRect.h * 31 / Settings::getGoodButtonYDivisor();
    rt = {left, top, screenRect.w * 35 / Settings::getGoodButtonXDivisor(),
          screenRect.h * 29 / Settings::getGoodButtonYDivisor()};
    for (auto &eP : equippable) {
        for (auto &e : eP) {
            boxes.push_back(e.getMaterial().button(
                false, e.getId(), e.getName(), e.getSplit(), rt, player->getNation()->getForeground(),
                player->getNation()->getBackground(), Settings::getEquipFontDivisor(), [this, e]() mutable {
                    player->equip(e);
                    setState(equipping);
                }));
            rt.y += dy;
        }
        rt.y = top;
        rt.x += dx;
    }
    // Create buttons for unequipping equipment
    rt.y = top;
    left = screenRect.w * 218 / Settings::getGoodButtonXDivisor();
    for (auto &e : player->getEquipment()) {
        auto &ss = e.getMaterial().getCombatStats();
        unsigned int pI = ss.front().partId;
        rt.x = left + static_cast<int>(pI) * dx;
        boxes.push_back(e.getMaterial().button(false, e.getId(), e.getName(), e.getSplit(), rt,
                                               player->getNation()->getForeground(), player->getNation()->getBackground(),
                                               screenRect.h / Settings::getEquipFontDivisor(), [this, pI, ss] {
                                                   player->unequip(pI);
                                                   // Equip fists.
                                                   for (auto &s : ss)
                                                       player->equip(s.partId);
                                                   setState(equipping);
                                               }));
    }
}

void Game::createManageButtons() {
    // Create buttons for managing businesses
    //     SDL_Rect rt = {screenRect.w / 15, screenRect.h / 15, screenRect.w * 2 / 5, screenRect.h * 4 / 5};
}

void Game::createAttackButton() {
    // Create button for selecting traveler to fight.
    SDL_Rect r = {screenRect.w * 2 / 3, screenRect.h * 14 / 15, 0, 0};
    std::vector<std::string> tx = {"Cancel (F)ight"};
    boxes.push_back(std::make_unique<MenuButton>(
        r, tx, Settings::getUIForeground(), Settings::getUIBackground(), Settings::getSmallBoxBorder(),
        Settings::getSmallBoxRadius(), screenRect.h / Settings::getSmallBoxFontDivisor(), [this] { setState(traveling); }));
    boxes.back()->setKey(SDLK_f);
    // Create vector of names of attackable travelers.
    auto able = player->attackable();
    std::vector<std::string> names;
    names.reserve(able.size());
    std::transform(able.begin(), able.end(), std::back_inserter(names),
                   [](const std::shared_ptr<Traveler> t) { return t->getName(); });
    // Create attack button.
    r = {screenRect.w / 4, screenRect.h / 4, screenRect.w / 2, screenRect.h / 2};
    boxes.push_back(std::make_unique<SelectButton>(r, names, player->getNation()->getForeground(),
                                                   player->getNation()->getBackground(), player->getNation()->getHighlight(),
                                                   Settings::getBigBoxBorder(), Settings::getBigBoxRadius(),
                                                   screenRect.h / Settings::getFightFontDivisor(), [this, able] {
                                                       int i = boxes.back()->getHighlightLine();
                                                       if (i > -1) {
                                                           player->attack(able[static_cast<size_t>(i)]);
                                                           setState(fighting);
                                                       } else
                                                           boxes.back()->setClicked(false);
                                                   }));
    pause = true;
    focus(0, Focusable::box);
}

void Game::createFightBoxes() {
    // Create buttons and text boxes for combat.
    auto target = player->getTarget().lock();
    if (not target) {
        setState(traveling);
        return;
    }
    SDL_Rect r = {screenRect.w / 2, screenRect.h / 4, 0, 0};
    std::vector<std::string> tx = {"Fighting " + target->getName() + "..."};
    boxes.push_back(std::make_unique<TextBox>(r, tx, player->getNation()->getForeground(),
                                              player->getNation()->getBackground(), Settings::getBigBoxBorder(),
                                              Settings::getBigBoxRadius(), screenRect.h / Settings::getFightFontDivisor()));
    r = {screenRect.w / 21, screenRect.h / 4, screenRect.w * 5 / 21, screenRect.h / 2};
    tx = {};
    boxes.push_back(std::make_unique<TextBox>(r, tx, player->getNation()->getForeground(),
                                              player->getNation()->getBackground(), Settings::getBigBoxBorder(),
                                              Settings::getBigBoxRadius(), screenRect.h / Settings::getFightFontDivisor()));
    r = {screenRect.w * 15 / 21, screenRect.h / 4, screenRect.w * 5 / 21, screenRect.h / 2};
    tx = {};
    boxes.push_back(std::make_unique<TextBox>(r, tx, target->getNation()->getForeground(),
                                              target->getNation()->getBackground(), Settings::getBigBoxBorder(),
                                              Settings::getBigBoxRadius(), screenRect.h / Settings::getFightFontDivisor()));
    r = {screenRect.w / 3, screenRect.h / 3, screenRect.w / 3, screenRect.h / 3};
    tx = {"Fight", "Run", "Yield"};
    boxes.push_back(std::make_unique<SelectButton>(r, tx, player->getNation()->getForeground(),
                                                   player->getNation()->getBackground(), player->getNation()->getHighlight(),
                                                   Settings::getBigBoxBorder(), Settings::getBigBoxRadius(),
                                                   screenRect.h / Settings::getFightFontDivisor(), [this] {
                                                       int choice = boxes[kFightChoiceIndex]->getHighlightLine();
                                                       if (choice > -1) {
                                                           player->choice = FightChoice(choice);
                                                           pause = false;
                                                       }
                                                   }));
    focus(kFightChoiceIndex, Focusable::box);
}

void Game::createLootButtons() {
    auto target = player->getTarget().lock();
    if (not target) {
        setState(traveling);
        return;
    }
    SDL_Rect rt = {screenRect.w / 15, screenRect.h * 14 / 15, 0, 0};
    std::vector<std::string> tx = {"(D)one Looting"};
    boxes.push_back(std::make_unique<MenuButton>(rt, tx, Settings::getUIForeground(), Settings::getUIBackground(),
                                                 Settings::getSmallBoxBorder(), Settings::getSmallBoxRadius(),
                                                 screenRect.h / Settings::getSmallBoxFontDivisor(), [this] {
                                                     player->loseTarget();
                                                     setState(traveling);
                                                 }));
    boxes.back()->setKey(SDLK_d);
    rt.x += screenRect.w / 5;
    tx = {"(L)oot All"};
    boxes.push_back(std::make_unique<MenuButton>(rt, tx, Settings::getUIForeground(), Settings::getUIBackground(),
                                                 Settings::getSmallBoxBorder(), Settings::getSmallBoxRadius(),
                                                 screenRect.h / Settings::getSmallBoxFontDivisor(), [this, &t = *target] {
                                                     player->loot(t);
                                                     player->loseTarget();
                                                     setState(traveling);
                                                 }));
    boxes.back()->setKey(SDLK_l);
    int left = screenRect.w / Settings::getGoodButtonXDivisor(), right = screenRect.w / 2;
    int top = screenRect.h / Settings::getGoodButtonYDivisor();
    int dx = screenRect.w * 31 / Settings::getGoodButtonXDivisor(),
        dy = screenRect.h * 31 / Settings::getGoodButtonYDivisor();
    rt = {left, top, screenRect.w * 29 / Settings::getGoodButtonXDivisor(),
          screenRect.h * 29 / Settings::getGoodButtonYDivisor()};
    for (auto &g : player->getGoods())
        for (auto &m : g.getMaterials()) {
            if ((m.getAmount() >= 0.01 and g.getSplit()) or (m.getAmount() >= 1)) {
                boxes.push_back(m.button(true, g.getId(), g.getName(), g.getSplit(), rt,
                                         player->getNation()->getForeground(), player->getNation()->getBackground(),
                                         screenRect.h / Settings::getTradeFontDivisor(), [this, &g, &m, &t = *target] {
                                             Good l(g.getId(), m.getAmount());
                                             l.addMaterial(Material(m.getId(), m.getAmount()));
                                             player->leave(l, t);
                                             setState(looting);
                                         }));
                rt.x += dx;
                if (rt.x + rt.w >= right) {
                    rt.x = left;
                    rt.y += dy;
                }
            }
        }
    left = screenRect.w / 2 + screenRect.w / Settings::getGoodButtonXDivisor();
    right = screenRect.w - screenRect.w / Settings::getGoodButtonXDivisor();
    rt.x = left;
    rt.y = top;
    for (auto &g : target->getGoods())
        for (auto &m : g.getMaterials()) {
            if ((m.getAmount() >= 0.01 and g.getSplit()) or (m.getAmount() >= 1)) {
                boxes.push_back(m.button(true, g.getId(), g.getName(), g.getSplit(), rt,
                                         player->getNation()->getForeground(), player->getNation()->getBackground(),
                                         screenRect.h / Settings::getTradeFontDivisor(), [this, &g, &m, &t = *target] {
                                             Good l(g.getId(), m.getAmount());
                                             l.addMaterial(Material(m.getId(), m.getAmount()));
                                             player->loot(l, t);
                                             setState(looting);
                                         }));
                rt.x += dx;
                if (rt.x + rt.w >= right) {
                    rt.x = left;
                    rt.y += dy;
                }
            }
        }
    pause = true;
}

void Game::createDyingBoxes() {
    SDL_Rect rt = {screenRect.w / 2, screenRect.h / 2, 0, 0};
    std::vector<std::string> tx = {player->getLogText().back(), "You have died."};
    boxes.push_back(std::make_unique<TextBox>(rt, tx, player->getNation()->getForeground(),
                                              player->getNation()->getBackground(), Settings::getBigBoxBorder(),
                                              Settings::getBigBoxRadius(), screenRect.h / Settings::getFightFontDivisor()));
}

void Game::handleEvents() {
    // Handle events since last poll.
    SDL_Event event;
    while (SDL_PollEvent(&event)) {
        double d;
        SDL_Keymod mod = SDL_GetModState();
        if ((mod & KMOD_CTRL) and (mod & KMOD_SHIFT) and (mod & KMOD_ALT))
            d = 10000;
        else if ((mod & KMOD_CTRL) and (mod & KMOD_ALT))
            d = 0.001;
        else if ((mod & KMOD_SHIFT) and (mod & KMOD_ALT))
            d = 0.01;
        else if ((mod & KMOD_CTRL) and (mod & KMOD_SHIFT))
            d = 1000;
        else if (mod & KMOD_ALT)
            d = 0.1;
        else if (mod & KMOD_SHIFT)
            d = 10;
        else if (mod & KMOD_CTRL)
            d = 100;
        else
            d = 1;
        Uint8 r, g, b;
        int mx, my, mw, mh;
        switch (event.type) {
        case SDL_KEYDOWN:
            if (not boxes.empty()) {
                auto keyedBox = std::find_if(boxes.begin(), boxes.end(), [&event](const std::unique_ptr<TextBox> &b) {
                    return event.key.keysym.sym == b->getKey();
                });
                if (keyedBox != boxes.end()) {
                    // A box's key was pressed.
                    focus(static_cast<int>(std::distance(boxes.begin(), keyedBox)), Focusable::box);
                    event.key.keysym.sym = SDLK_SPACE;
                }
            }
            if (not(focusBox > -1 and boxes[static_cast<size_t>(focusBox)]->keyCaptured(event.key))) {
                if (state != quitting) {
                    if (player) {
                        switch (state) {
                        case traveling:
                            switch (event.key.keysym.sym) {
                            case SDLK_LEFT:
                                scrollX = static_cast<int>(static_cast<double>(-scrollSpeed) * d);
                                showPlayer = false;
                                break;
                            case SDLK_RIGHT:
                                scrollX = static_cast<int>(static_cast<double>(scrollSpeed) * d);
                                showPlayer = false;
                                break;
                            case SDLK_UP:
                                scrollY = static_cast<int>(static_cast<double>(-scrollSpeed) * d);
                                showPlayer = false;
                                break;
                            case SDLK_DOWN:
                                scrollY = static_cast<int>(static_cast<double>(scrollSpeed) * d);
                                showPlayer = false;
                                break;
                            case SDLK_s:
                                focusPrev(Focusable::neighbor);
                                break;
                            case SDLK_d:
                                focusNext(Focusable::neighbor);
                                break;
                            }
                            break;
                        case trading:
                            switch (event.key.keysym.sym) {
                            case SDLK_LEFT:
                                focusPrev(Focusable::box);
                                break;
                            case SDLK_RIGHT:
                                focusNext(Focusable::box);
                                break;
                            case SDLK_UP:
                                for (int i = 0; i < 7; ++i)
                                    focusPrev(Focusable::box);
                                break;
                            case SDLK_DOWN:
                                for (int i = 0; i < 7; ++i)
                                    focusNext(Focusable::box);
                                break;
                            case SDLK_COMMA:
                                player->changeTradePortion(-0.1);
                                boxes[kTradePortionIndex]->setText({player->tradePortionString()});
                                break;
                            case SDLK_PERIOD:
                                player->changeTradePortion(0.1);
                                boxes[kTradePortionIndex]->setText({player->tradePortionString()});
                                break;
                            case SDLK_c:
                                player->makeTrade();
                                setState(trading);
                                break;
                            case SDLK_KP_MINUS:
                                player->adjustAreas(boxes, -d);
                                break;
                            case SDLK_KP_PLUS:
                                player->adjustAreas(boxes, d);
                                break;
                            case SDLK_m:
                                saveData();
                                break;
                            case SDLK_o:
                                player->adjustDemand(boxes, -d);
                                break;
                            case SDLK_p:
                                player->adjustDemand(boxes, d);
                                break;
                            case SDLK_r:
                                player->resetTown();
                                break;
                            case SDLK_x:
                                player->toggleMaxGoods();
                                break;
                            }
                            break;
                        default:
                            break;
                        }
                    }
                }
                switch (event.key.keysym.sym) {
                case SDLK_ESCAPE:
                    toggleState(quitting);
                    break;
                case SDLK_TAB:
                    if (SDL_GetModState() & KMOD_SHIFT)
                        focusPrev(Focusable::box);
                    else
                        focusNext(Focusable::box);
                    break;
                case SDLK_n:
                    saveRoutes();
                    break;
                }
            } else
                boxes[static_cast<size_t>(focusBox)]->handleKey(event.key);
            break;
        case SDL_KEYUP:
            if (not boxes.empty()) {
                auto keyedBox = std::find_if(boxes.begin(), boxes.end(), [&event](const std::unique_ptr<TextBox> &b) {
                    return event.key.keysym.sym == b->getKey();
                });
                if (keyedBox != boxes.end())
                    // A box's key was released.
                    event.key.keysym.sym = SDLK_SPACE;
            }
            if (not(focusBox > -1 and boxes[static_cast<size_t>(focusBox)]->keyCaptured(event.key)))
                switch (event.key.keysym.sym) {
                case SDLK_LEFT:
                case SDLK_RIGHT:
                    scrollX = 0;
                    break;
                case SDLK_UP:
                case SDLK_DOWN:
                    scrollY = 0;
                    break;
                }
            else
                boxes[static_cast<size_t>(focusBox)]->handleKey(event.key);
            break;
        case SDL_TEXTINPUT:
            if (focusBox > -1)
                boxes[static_cast<size_t>(focusBox)]->handleTextInput(event.text);
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
            __attribute__((fallthrough));
        case SDL_MOUSEBUTTONUP:
            if (not towns.empty() and state == traveling) {
                auto clickedTown = std::find_if(towns.begin(), towns.end(),
                                                [&event](const Town &t) { return t.clickCaptured(event.button); });
                if (clickedTown != towns.end())
                    focus(static_cast<int>(std::distance(towns.begin(), clickedTown)), Focusable::town);
            }
            if (not boxes.empty()) {
                auto clickedBox = std::find_if(boxes.begin(), boxes.end(), [&event](const std::unique_ptr<TextBox> &b) {
                    return b->getCanFocus() and b->clickCaptured(event.button);
                });
                if (clickedBox != boxes.end()) {
                    int cI = static_cast<int>(std::distance(boxes.begin(), clickedBox));
                    if (cI != focusBox)
                        focus(cI, Focusable::box);
                    (*clickedBox)->handleClick(event.button);
                } else
                    focus(-1, Focusable::box);
            }
            break;
        case SDL_QUIT:
            stop = true;
            break;
        }
    }
}

void Game::update() {
    currentTime = SDL_GetTicks();
    unsigned int elapsed = currentTime - lastTime;
    lastTime = currentTime;
    if (showPlayer) {
        scrollX = 0;
        scrollY = 0;
        if (player->getPX() < int(mapRect.w * kShowPlayerPadding))
            scrollX = -scrollSpeed;
        if (player->getPY() < int(mapRect.h * kShowPlayerPadding))
            scrollY = -scrollSpeed;
        if (player->getPX() > int(mapRect.w * (1 - kShowPlayerPadding)))
            scrollX = scrollSpeed;
        if (player->getPY() > int(mapRect.h * (1 - kShowPlayerPadding)))
            scrollY = scrollSpeed;
    }
    if (scrollX or scrollY)
        changeOffsets(scrollX, scrollY);
    if (not(pause or travelers.empty())) {
        for (auto &t : towns)
            t.update(elapsed);
        for (auto tI = travelers.begin() + 1; tI != travelers.end(); ++tI)
            (*tI)->runAI(elapsed);
        for (auto &t : travelers)
            t->update(elapsed);
        for (auto &t : travelers)
            t->place(offsetX, offsetY, scale);
    }
    updateUI();
}

void Game::draw() {
    // Draw UI and game elements.
    SDL_RenderCopy(screen, mapTexture, &mapRect, nullptr);
    for (auto &t : towns)
        t.drawRoutes(screen);
    for (auto &t : towns)
        t.drawDot(screen);
    for (auto &t : travelers)
        t->draw(screen);
    for (auto &t : towns)
        t.drawText(screen);
    for (auto &b : boxes)
        b->draw(screen);
    SDL_RenderPresent(screen);
}

void Game::saveRoutes() {
    // Recalculate routes between towns and save new routes to database.
    LoadBar loadBar({screenRect.w / 15, screenRect.h * 7 / 15, screenRect.w * 13 / 15, screenRect.h / 15},
                    {"Finding routes..."}, {0, 255, 0, 255}, Settings::getUIForeground(), Settings::getBigBoxRadius(),
                    Settings::getBigBoxBorder(), Settings::getLoadBarDivisor());
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
    if (not travelers.empty())
        for (auto tI = travelers.begin() + 1; tI != travelers.end(); ++tI)
            (*tI)->startAI();
    sqlite3_finalize(comm);
    sqlite3_close(conn);
}

void Game::saveData() {
    // Save changes to frequencies and demand slopes to database.
    sqlite3 *conn;
    if (sqlite3_open("1025ad.db", &conn) != SQLITE_OK)
        std::cout << "Error opening sqlite database:" << sqlite3_errmsg(conn) << std::endl;
    std::string updates = "UPDATE frequencies SET frequency = CASE";
    travelers.front()->getTown()->saveFrequencies(updates);
    sqlite3_stmt *comm;
    sqlite3_prepare_v2(conn, updates.c_str(), -1, &comm, nullptr);
    if (sqlite3_step(comm) != SQLITE_DONE)
        std::cout << "Error updating frequencies: " << sqlite3_errmsg(conn) << std::endl << updates << std::endl;
    sqlite3_finalize(comm);
    updates = "UPDATE consumption SET demand_slope = CASE";
    travelers.front()->getTown()->saveDemand(updates);
    sqlite3_prepare_v2(conn, updates.c_str(), -1, &comm, nullptr);
    if (sqlite3_step(comm) != SQLITE_DONE)
        std::cout << "Error updating demand slopes: " << sqlite3_errmsg(conn) << std::endl << updates << std::endl;
    sqlite3_finalize(comm);
    if (sqlite3_close(conn) != SQLITE_OK)
        std::cout << sqlite3_errmsg(conn) << std::endl;
}

void Game::save() {
    // Save the game.
    flatbuffers::FlatBufferBuilder builder(1024);
    auto sTowns = builder.CreateVector<flatbuffers::Offset<Save::Town>>(
        towns.size(), [this, &builder](size_t i) { return towns[i].save(builder); });
    auto sTravelers = builder.CreateVector<flatbuffers::Offset<Save::Traveler>>(
        travelers.size(), [this, &builder](size_t i) { return travelers[i]->save(builder); });
    auto game = Save::CreateGame(builder, sTowns, sTravelers);
    builder.Finish(game);
    fs::path path("save");
    path /= travelers.front()->getName();
    path.replace_extension("sav");
    std::ofstream file(path.string(), std::ofstream::binary);
    if (file.is_open())
        file.write((const char *)builder.GetBufferPointer(), builder.GetSize());
}
