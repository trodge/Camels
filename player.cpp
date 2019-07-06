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

#include "player.hpp"

Player::Player(Game *g) : game(g) {}

void Player::loadTraveler(const Save::Traveler *t, std::vector<Town> &ts) {
    // Load the traveler for the player from save file.
    traveler = std::make_shared<Traveler>(t, ts, game->getNations(), game->getData());
}

void Player::prepFocus(Focusable::FocusGroup g, int &i, int &s, std::vector<Focusable *> &fcbls) {
    std::vector<Town *> neighbors;
    switch (g) {
    case Focusable::box:
        i = focusBox;
        s = static_cast<int>(boxes.size());
        for (auto &b : boxes)
            fcbls.push_back(b.get());
        break;
    case Focusable::neighbor:
        neighbors = traveler->getTown()->getNeighbors();
        i = static_cast<int>(
            std::find_if(neighbors.begin(), neighbors.end(),
                         [this](Town *t) { return t->getId() == static_cast<unsigned int>(focusTown + 1); }) -
            neighbors.begin());
        s = static_cast<int>(neighbors.size());
        if (i == s) {
            if (focusTown > -1 and static_cast<size_t>(focusTown) < game->getTowns().size())
                game->unFocusTown(static_cast<size_t>(focusTown));
            i = -1;
        }
        fcbls = std::vector<Focusable *>(neighbors.begin(), neighbors.end());
        break;
    case Focusable::town:
        i = focusTown;
        game->fillFocusableTowns(fcbls);
        s = static_cast<int>(fcbls.size());
        break;
    }
}

void Player::finishFocus(int f, Focusable::FocusGroup g, const std::vector<Focusable *> &fcbls) {
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

void Player::focus(int f, Focusable::FocusGroup g) {
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

void Player::focusPrev(Focusable::FocusGroup g) {
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

void Player::focusNext(Focusable::FocusGroup g) {
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

void Player::setState(UIState s) {
    // Change the UI state to s.
    boxes.clear();
    focusBox = -1;
    switch (s) {
    case starting:
        createStartButtons();
        break;
    case beginning:
        createBeginButtons();
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
    case storing:
        createStorageButtons();
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

void Player::toggleState(UIState s) {
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


void Player::handleKey(const SDL_KeyboardEvent &k) {
    SDL_Keymod mod = SDL_GetModState();
    double d;
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
    if (k.state == SDL_PRESSED) {
        if (not boxes.empty()) {
            auto keyedBox = std::find_if(boxes.begin(), boxes.end(),
                                         [&k](const std::unique_ptr<TextBox> &t) { return k.keysym.sym == t->getKey(); });
            if (keyedBox != boxes.end()) {
                // A box's key was pressed.
                focus(static_cast<int>(std::distance(boxes.begin(), keyedBox)), Focusable::box);
                (*keyedBox)->handleKey(k);
            }
        }
        if (not(focusBox > -1 and boxes[static_cast<size_t>(focusBox)]->keyCaptured(k))) {
            if (state != quitting) {
                if (traveler) {
                    double dSS = static_cast<double>(Settings::getScroll());
                    switch (state) {
                    case traveling:
                        switch (k.keysym.sym) {
                        case SDLK_LEFT:
                            scrollX = static_cast<int>(-dSS * d);
                            show = false;
                            break;
                        case SDLK_RIGHT:
                            scrollX = static_cast<int>(dSS * d);
                            show = false;
                            break;
                        case SDLK_UP:
                            scrollY = static_cast<int>(-dSS * d);
                            show = false;
                            break;
                        case SDLK_DOWN:
                            scrollY = static_cast<int>(dSS * d);
                            show = false;
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
                        switch (k.keysym.sym) {
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
                            traveler->changePortion(-0.1);
                            updatePortionBox();
                            break;
                        case SDLK_PERIOD:
                            traveler->changePortion(0.1);
                            updatePortionBox();
                            break;
                        case SDLK_KP_MINUS:
                            traveler->adjustAreas(boxes, requestButtonIndex, -d);
                            break;
                        case SDLK_KP_PLUS:
                            traveler->adjustAreas(boxes, requestButtonIndex, d);
                            break;
                        case SDLK_m:
                            game->saveData();
                            break;
                        case SDLK_o:
                            traveler->adjustDemand(boxes, requestButtonIndex, -d);
                            break;
                        case SDLK_p:
                            traveler->adjustDemand(boxes, requestButtonIndex, d);
                            break;
                        case SDLK_r:
                            traveler->resetTown();
                            break;
                        case SDLK_x:
                            traveler->toggleMaxGoods();
                            break;
                        }
                        break;
                    default:
                        break;
                    }
                }
            }
            switch (k.keysym.sym) {
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
                game->saveRoutes();
                break;
            }
        } else
            boxes[static_cast<size_t>(focusBox)]->handleKey(k);
    } else {
        if (not boxes.empty()) {
            auto keyedBox = std::find_if(boxes.begin(), boxes.end(),
                                         [&k](const std::unique_ptr<TextBox> &t) { return k.keysym.sym == t->getKey(); });
            if (keyedBox != boxes.end())
                // A box's key was released.
                (*keyedBox)->handleKey(k);
        }
        if (not(focusBox > -1 and boxes[static_cast<size_t>(focusBox)]->keyCaptured(k)))
            switch (k.keysym.sym) {
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
            boxes[static_cast<size_t>(focusBox)]->handleKey(k);
    }
}

void Player::handleTextInput(const SDL_TextInputEvent &t) {
    if (focusBox > -1)
        boxes[static_cast<size_t>(focusBox)]->handleTextInput(t);
}

void Player::handleClick(const SDL_MouseButtonEvent &b) {
    if (state == traveling) {
        std::vector<Focusable *> fcbls;
        game->fillFocusableTowns(fcbls);
        auto clickedTown = std::find_if(fcbls.begin(), fcbls.end(), [&b](const Focusable *t) { return t->clickCaptured(b); });
        if (clickedTown != fcbls.end())
            focus(static_cast<int>(std::distance(fcbls.begin(), clickedTown)), Focusable::town);
    }
    if (not boxes.empty()) {
        auto clickedBox = std::find_if(boxes.begin(), boxes.end(), [&b](const std::unique_ptr<TextBox> &t) {
            return t->getCanFocus() and t->clickCaptured(b);
        });
        if (clickedBox != boxes.end()) {
            int cI = static_cast<int>(std::distance(boxes.begin(), clickedBox));
            if (cI != focusBox)
                focus(cI, Focusable::box);
            (*clickedBox)->handleClick(b);
        } else
            focus(-1, Focusable::box);
    }
}

void Player::handleEvent(const SDL_Event &e) {
    switch(e.type) {
    case SDL_KEYDOWN:
        __attribute__((fallthrough));
    case SDL_KEYUP:
        handleKey(e.key);
        break;
    case SDL_TEXTINPUT:
        handleTextInput(e.text);
        break;
    case SDL_MOUSEBUTTONDOWN:
        __attribute__((fallthrough));
    case SDL_MOUSEBUTTONUP:
        handleClick(e.button);
        break;
    case SDL_QUIT:
        stop = true;
        break;
    }
}

void Player::update(unsigned int e) {
    // Update the UI to reflect current state.
    auto t = traveler.get();
    bool target = t and t->getTarget().lock();
    switch (state) {
    case traveling:
        if (target) {
            pause = true;
            setState(fighting);
        }
        break;
    case trading:
        updateTradeButtons();
        break;
    case fighting:
        if (not traveler->alive()) {
            setState(dying);
            break;
        } else if (not target) {
            setState(logging);
            break;
        } else if (traveler->fightWon()) {
            setState(looting);
            break;
        }
        updateFightBoxes();
        break;
    default:
        break;
    }
    if (show) {
        scrollX = 0;
        scrollY = 0;
        const SDL_Rect &mR = game->getMapRect();
        int sS = Settings::getScroll();
        if (traveler->getPX() < int(mR.w * kShowPlayerPadding))
            scrollX = -sS;
        if (traveler->getPY() < int(mR.h * kShowPlayerPadding))
            scrollY = -sS;
        if (traveler->getPX() > int(mR.w * (1 - kShowPlayerPadding)))
            scrollX = sS;
        if (traveler->getPY() > int(mR.h * (1 - kShowPlayerPadding)))
            scrollY = sS;
    }
    if (scrollX or scrollY)
        game->moveView(scrollX, scrollY);
    if (traveler.get() and not pause)
        traveler->update(e);
}

void Player::createStartButtons() {
    const SDL_Rect &sR = Settings::getScreenRect();
    SDL_Rect rt = {sR.w / 2, sR.h / 15, 0, 0};
    std::vector<std::string> tx = {"Camels and Silk"};
    boxes.push_back(std::make_unique<TextBox>(rt, tx, Settings::getUIForeground(), Settings::getUIBackground(), Settings::getBigBoxBorder(),
                                              Settings::getBigBoxRadius(), Settings::getBigBoxFontSize()));
    rt = {sR.w / 7, sR.h / 3, 0, 0};
    tx = {"(N)ew Game"};
    boxes.push_back(std::make_unique<MenuButton>(rt, tx, Settings::getUIForeground(), Settings::getUIBackground(),
                                                 Settings::getBigBoxBorder(), Settings::getBigBoxRadius(), Settings::getBigBoxFontSize(),
                                                 [this] {
                                                     game->newGame();
                                                     setState(beginning);
                                                 }));
    boxes.back()->setKey(SDLK_n);
    rt = {sR.w / 7, sR.h * 2 / 3, 0, 0};
    tx = {"(L)oad Game"};
    boxes.push_back(std::make_unique<MenuButton>(rt, tx, Settings::getUIForeground(), Settings::getUIBackground(),
                                                 Settings::getBigBoxBorder(), Settings::getBigBoxRadius(), Settings::getBigBoxFontSize(),
                                                 [this] {
                                                     boxes.back()->setClicked(false);
                                                     toggleState(loading);
                                                 }));
    boxes.back()->setKey(SDLK_l);
}

void Player::createBeginButtons() {
    const SDL_Rect &sR = Settings::getScreenRect();
    SDL_Rect rt = {sR.w / 2, sR.h / 7, 0, 0};
    // Create textbox for entering name.
    std::vector<std::string> tx = {"Name", ""};
    boxes.push_back(std::make_unique<TextBox>(rt, tx, Settings::getUIForeground(), Settings::getUIBackground(), Settings::getBigBoxBorder(),
                                              Settings::getBigBoxRadius(), Settings::getBigBoxFontSize()));
    boxes.back()->toggleLock();
    for (auto &n : game->getNations()) {
        // Create a button for each nation to start in that nation.
        rt = {sR.w * (static_cast<int>(n.getId() - 1) % 3 * 2 + 1) / 6, sR.h * (static_cast<int>(n.getId() - 1) / 3 + 2) / 7,
              0, 0};
        boxes.push_back(std::make_unique<MenuButton>(rt, n.getNames(), n.getForeground(), n.getBackground(), n.getId(), true,
                                                     3, Settings::getBigBoxRadius(), Settings::getBigBoxFontSize(), [this, n] {
                                                         focusBox = -1;
                                                         unsigned int nId = n.getId(); // nation id
                                                         std::string name = boxes.front()->getText(1);
                                                         if (name.empty())
                                                             name = n.randomName();
                                                         // Create traveler object for player
                                                         traveler = game->createPlayerTraveler(nId, name);
                                                         show = true;
                                                         setState(traveling);
                                                     }));
    }
}

void Player::createQuitButtons() {
    // Bring up quit menu.
    const SDL_Rect &sR = Settings::getScreenRect();
    SDL_Rect rt = {sR.w / 2, sR.h / 4, 0, 0};
    std::vector<std::string> tx = {"Continue"};
    boxes.push_back(std::make_unique<MenuButton>(rt, tx, Settings::getUIForeground(), Settings::getUIBackground(),
                                                 Settings::getBigBoxBorder(), Settings::getBigBoxRadius(), Settings::getBigBoxFontSize(),
                                                 [this] { toggleState(quitting); }));
    rt = {sR.w / 2, sR.h * 3 / 4, 0, 0};
    std::function<void()> f;
    if (traveler) {
        tx = {"Save and Quit"};
        f = [this] {
            game->saveGame();
            stop = true;
        };
    } else {
        tx = {"Quit"};
        f = [this] { stop = true; };
    }
    boxes.push_back(std::make_unique<MenuButton>(rt, tx, Settings::getUIForeground(), Settings::getUIBackground(),
                                                 Settings::getBigBoxBorder(), Settings::getBigBoxRadius(), Settings::getBigBoxFontSize(),
                                                 f));
    focus(1, Focusable::box);
}

void Player::createLoadButtons() {
    const SDL_Rect &sR = Settings::getScreenRect();
    SDL_Rect rt = {sR.w / 7, sR.h / 7, 0, 0};
    std::vector<std::string> tx = {"(B)ack"};
    boxes.push_back(std::make_unique<MenuButton>(rt, tx, Settings::getUIForeground(), Settings::getUIBackground(),
                                                 Settings::getBigBoxBorder(), Settings::getBigBoxRadius(), Settings::getBigBoxFontSize(),
                                                 [this] { toggleState(loading); }));
    boxes.back()->setKey(SDLK_b);
    fs::path path = "save";
    std::vector<std::string> saves;
    for (auto &f : fs::directory_iterator(path))
        saves.push_back(f.path().stem().string());
    rt = {sR.w / 2, sR.h / 15, 0, 0};
    tx = {"Load"};
    boxes.push_back(std::make_unique<TextBox>(rt, tx, Settings::getUIForeground(), Settings::getUIBackground(), Settings::getBigBoxBorder(),
                                              Settings::getBigBoxRadius(), Settings::getBigBoxFontSize()));
    rt = {sR.w / 5, sR.h / 7, sR.w * 3 / 5, sR.h * 5 / 7};
    boxes.push_back(std::make_unique<SelectButton>(
        rt, saves, Settings::getUIForeground(), Settings::getUIBackground(), Settings::getUIHighlight(), Settings::getBigBoxBorder(),
        Settings::getBigBoxRadius(), Settings::getBigBoxFontSize(),
        [this, path] { game->loadGame((path / boxes.back()->getItem()).replace_extension("sav")); }));
    focus(2, Focusable::box);
}

void Player::createTravelButtons() {
    const SDL_Rect &sR = Settings::getScreenRect();
    SDL_Rect rt = {sR.w / 6, sR.h * 14 / 15, 0, 0};
    std::vector<std::string> tx = {"(T)rade"};
    boxes.push_back(std::make_unique<MenuButton>(rt, tx, Settings::getUIForeground(), Settings::getUIBackground(),
                                                 Settings::getSmallBoxBorder(), Settings::getSmallBoxRadius(),
                                                 Settings::getSmallBoxFontSize(), [this] { setState(trading); }));
    boxes.back()->setKey(SDLK_t);
    rt.x += sR.w / 6;
    tx = {"(G)o"};
    boxes.push_back(std::make_unique<MenuButton>(rt, tx, Settings::getUIForeground(), Settings::getUIBackground(),
                                                 Settings::getSmallBoxBorder(), Settings::getSmallBoxRadius(),
                                                 Settings::getSmallBoxFontSize(), [this] {
                                                     if (focusTown > -1)
                                                         game->pickTown(traveler, static_cast<size_t>(focusTown));
                                                     show = true;
                                                     boxes[1]->setClicked(false);
                                                 }));
    boxes.back()->setKey(SDLK_g);
    rt.x += sR.w / 6;
    tx = {"(E)quip"};
    boxes.push_back(std::make_unique<MenuButton>(rt, tx, Settings::getUIForeground(), Settings::getUIBackground(),
                                                 Settings::getSmallBoxBorder(), Settings::getSmallBoxRadius(),
                                                 Settings::getSmallBoxFontSize(), [this] { setState(equipping); }));
    boxes.back()->setKey(SDLK_e);
    rt.x += sR.w / 6;
    tx = {"(F)ight"};
    boxes.push_back(std::make_unique<MenuButton>(rt, tx, Settings::getUIForeground(), Settings::getUIBackground(),
                                                 Settings::getSmallBoxBorder(), Settings::getSmallBoxRadius(),
                                                 Settings::getSmallBoxFontSize(), [this] { setState(attacking); }));
    boxes.back()->setKey(SDLK_f);
    rt.x += sR.w / 6;
    tx = {"View (L)og"};
    boxes.push_back(std::make_unique<MenuButton>(rt, tx, Settings::getUIForeground(), Settings::getUIBackground(),
                                                 Settings::getSmallBoxBorder(), Settings::getSmallBoxRadius(),
                                                 Settings::getSmallBoxFontSize(), [this] { setState(logging); }));
    boxes.back()->setKey(SDLK_l);
    pause = false;
    focus(0, Focusable::box);
}

void Player::createLogBox() {
    // Create text boxes for viewing log.
    const SDL_Rect &sR = Settings::getScreenRect();
    SDL_Rect rt = {sR.w * 5 / 6, sR.h * 14 / 15, 0, 0};
    std::vector<std::string> tx = {"Close (L)og"};
    boxes.push_back(std::make_unique<MenuButton>(rt, tx, Settings::getUIForeground(), Settings::getUIBackground(),
                                                 Settings::getSmallBoxBorder(), Settings::getSmallBoxRadius(),
                                                 Settings::getSmallBoxFontSize(), [this] { setState(traveling); }));
    boxes.back()->setKey(SDLK_l);
    // Create log box.
    rt = {sR.w / 15, sR.h * 2 / 15, sR.w * 13 / 15, sR.h * 11 / 15};
    boxes.push_back(std::make_unique<ScrollBox>(
        rt, traveler->getLogText(), traveler->getNation()->getForeground(), traveler->getNation()->getBackground(),
        traveler->getNation()->getHighlight(), Settings::getBigBoxBorder(), Settings::getBigBoxRadius(), Settings::getFightFontSize()));
    boxes.back()->setHighlightLine(-1);
}

void Player::createTradeButtons() {
    // Create buttons for trading
    const SDL_Rect &sR = Settings::getScreenRect();
    SDL_Rect rt = {sR.w / 6, sR.h * 14 / 15, 0, 0};
    std::vector<std::string> tx = {"Stop (T)rading"};
    const SDL_Color &uIFgr = Settings::getUIForeground(), &uIBgr = Settings::getUIBackground();
    int b = Settings::getSmallBoxBorder(), r = Settings::getSmallBoxRadius(), fS = Settings::getSmallBoxFontSize();
    boxes.push_back(std::make_unique<MenuButton>(rt, tx, uIFgr, uIBgr, b, r, fS, [this] { setState(traveling); }));
    boxes.back()->setKey(SDLK_t);
    focus(0, Focusable::box);
    rt.y -= boxes.back()->getRect().h * 3 / 2;
    tx = {"(C)omplete Trade"};
    boxes.push_back(std::make_unique<MenuButton>(rt, tx, uIFgr, uIBgr, b, r, fS, [this] {
        traveler->makeTrade();
        setState(trading);
    }));
    boxes.back()->setKey(SDLK_c);
    rt.x += sR.w / 6;
    tx = {traveler->portionString()};
    portionBoxIndex = boxes.size();
    boxes.push_back(std::make_unique<TextBox>(rt, tx, uIFgr, uIBgr, b, r, fS));
    boxes.back()->toggleLock();
    rt.y += boxes.back()->getRect().h * 3 / 2;
    tx = {"(S)et Portion"};
    setPortionButtonIndex = boxes.size();
    boxes.push_back(std::make_unique<MenuButton>(rt, tx, uIFgr, uIBgr, b, r, fS, [this] {
        double p;
        std::stringstream(boxes[portionBoxIndex]->getText().back()) >> p;
        traveler->setPortion(p);
        updatePortionBox();
        boxes[setPortionButtonIndex]->setClicked(false);
    }));
    boxes.back()->setKey(SDLK_s);
    // Create buttons for trading goods.
    int gBXD = Settings::getGoodButtonXDivisor(), gBYD = Settings::getGoodButtonYDivisor();
    // function to call when buttons are clicked
    auto f = [this] { updateTradeButtons(); };
    // Create the offer and request buttons for the player.
    int left = sR.w / gBXD, right = sR.w / 2;
    int top = sR.h / gBXD;
    int dx = sR.w * 31 / gBXD, dy = sR.h * 31 / gBYD;
    rt = {left, top, sR.w * 29 / gBXD, sR.h * 29 / gBYD};
    const SDL_Color &fgr = traveler->getNation()->getForeground(), &bgr = traveler->getNation()->getBackground();
    b = Settings::getTradeBorder();
    r = Settings::getTradeRadius();
    fS = Settings::getTradeFontSize();
    offerButtonIndex = boxes.size();
    for (auto &g : traveler->getGoods()) {
        for (auto &m : g.getMaterials())
            if ((m.getAmount() >= 0.01 and g.getSplit()) or (m.getAmount() >= 1)) {
                boxes.push_back(m.button(true, g.getId(), g.getName(), g.getSplit(), rt, fgr, bgr, b, r, fS, game->getData(), f));
                rt.x += dx;
                if (rt.x + rt.w >= right) {
                    rt.x = left;
                    rt.y += dy;
                }
            }
    }
    left = sR.w / 2 + sR.w / gBXD;
    right = sR.w - sR.w / gBXD;
    rt.x = left;
    rt.y = top;
    requestButtonIndex = boxes.size();
    const SDL_Color &tFgr = traveler->getTown()->getNation()->getForeground(),
                    &tBgr = traveler->getTown()->getNation()->getBackground();
    for (auto &g : traveler->getTown()->getGoods()) {
        for (auto &m : g.getMaterials())
            if ((m.getAmount() >= 0.00 and g.getSplit()) or (m.getAmount() >= 0)) {
                boxes.push_back(m.button(true, g.getId(), g.getName(), g.getSplit(), rt, tFgr, tBgr, b, r, fS, game->getData(), f));
                rt.x += dx;
                if (rt.x + rt.w >= right) {
                    rt.x = left;
                    rt.y += dy;
                }
            }
    }
}

void Player::updateTradeButtons() {
    // Update the values shown on trade portion, offer, and request boxes and set offer and request.
    std::string bN;
    double offerValue = 0;
    auto rBB = boxes.begin() + static_cast<std::vector<std::unique_ptr<TextBox>>::difference_type>(
                                   requestButtonIndex); // iterator to first request button
    // Loop through all boxes after cancel button and before first request button.
    for (auto oBI = boxes.begin() + static_cast<std::vector<std::unique_ptr<TextBox>>::difference_type>(offerButtonIndex);
         oBI != rBB; ++oBI) {
        auto &g = traveler->getGood((*oBI)->getId()); // good corresponding to oBI
        auto &gMs = g.getMaterials();
        bN = (*oBI)->getText()[0]; // first name on button
        auto mI = std::find_if(gMs.begin(), gMs.end(), [bN](const Material &m) {
            const std::string &mN = m.getName();
            return mN.substr(0, mN.find(' ')) == bN.substr(0, bN.find(' '));
        });
        if (mI != gMs.end()) {
            // mI is iterator to the material on oBI
            mI->updateButton(g.getSplit(), 0, 0, oBI->get());
            if ((*oBI)->getClicked()) {
                double amount = traveler->offerGood(g, *mI);
                // Use town's version of same material to get price.
                auto &tM = traveler->getTown()->getGood(g.getId()).getMaterial(*mI);
                offerValue += tM.getPrice(amount);
            }
        }
    }
    int requestCount =
        std::accumulate(rBB, boxes.end(), 0, [](int c, const std::unique_ptr<TextBox> &rB) { return c + rB->getClicked(); });
    double excess = 0; // excess value of offer over value needed for request
    // Loop through request buttons.
    for (auto rBI = rBB; rBI != boxes.end(); ++rBI) {
        auto &g = traveler->getTown()->getGood((*rBI)->getId()); // good in town corresponding to rBI
        auto &gMs = g.getMaterials();
        bN = (*rBI)->getText()[0];
        auto mI = std::find_if(gMs.begin(), gMs.end(), [bN](Material m) {
            const std::string &mN = m.getName();
            return mN.substr(0, mN.find(' ')) == bN.substr(0, bN.find(' '));
        });
        if (mI != gMs.end()) {
            // mI is iterator to the material on rBI
            mI->updateButton(g.getSplit(), offerValue, requestCount ? requestCount : 1, rBI->get());
            if ((*rBI)->getClicked()) {
                // Convert material excess to value and add to overall excess.
                excess += mI->getPrice(traveler->requestGood(g, *mI, offerValue, requestCount));
            }
        }
    }
    traveler->divideExcess(excess);
}

void Player::updatePortionBox() { boxes[portionBoxIndex]->setText(0, traveler->portionString()); }

void Player::createEquipButtons() {
    // Create buttons for equipping and unequipping items.
    const SDL_Rect &sR = Settings::getScreenRect();
    SDL_Rect rt = {sR.w / 2, sR.h * 14 / 15, 0, 0};
    std::vector<std::string> tx = {"Stop (E)quipping"};
    boxes.push_back(std::make_unique<MenuButton>(rt, tx, Settings::getUIForeground(), Settings::getUIBackground(),
                                                 Settings::getSmallBoxBorder(), Settings::getSmallBoxRadius(),
                                                 Settings::getSmallBoxFontSize(), [this] { setState(traveling); }));
    boxes.back()->setKey(SDLK_e);
    focus(0, Focusable::box);
    // Create array of vectors of equippable goods corresponding to each part that can hold equipment.
    std::array<std::vector<Good>, 6> equippable;
    for (auto &g : traveler->getGoods())
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
    int gBXD = Settings::getGoodButtonXDivisor(), gBYD = Settings::getGoodButtonYDivisor();
    // Create buttons for equipping equippables
    int left = sR.w / gBXD, top = sR.h / gBYD;
    int dx = sR.w * 36 / gBXD, dy = sR.h * 31 / gBYD;
    rt = {left, top, sR.w * 35 / gBXD, sR.h * 29 / gBYD};
    const SDL_Color &fgr = traveler->getNation()->getForeground(), &bgr = traveler->getNation()->getBackground();
    int b = Settings::getEquipBorder(), r = Settings::getEquipRadius(), fS = Settings::getEquipFontSize();
    equipButtonIndex = boxes.size();
    for (auto &eP : equippable) {
        for (auto &e : eP) {
            boxes.push_back(e.getMaterial().button(false, e.getId(), e.getName(), e.getSplit(), rt, fgr, bgr, b, r, fS,
                                                   game->getData(), [this, e]() mutable {
                                                       traveler->equip(e);
                                                       // Refresh buttons.
                                                       setState(equipping);
                                                   }));
            rt.y += dy;
        }
        rt.y = top;
        rt.x += dx;
    }
    // Create buttons for unequipping equipment
    rt.y = top;
    left = sR.w * 218 / gBXD;
    for (auto &e : traveler->getEquipment()) {
        auto &ss = e.getMaterial().getCombatStats();
        unsigned int pI = ss.front().partId;
        rt.x = left + static_cast<int>(pI) * dx;
        boxes.push_back(e.getMaterial().button(false, e.getId(), e.getName(), e.getSplit(), rt, fgr, bgr, b, r, fS, game->getData(),
                                               [this, pI, ss] {
                                                   traveler->unequip(pI);
                                                   // Equip fists.
                                                   for (auto &s : ss)
                                                       traveler->equip(s.partId);
                                                   // Refresh buttons.
                                                   setState(equipping);
                                               }));
    }
}

void Player::createStorageButtons() {
    // Create buttons for storing goods.
    const SDL_Rect &sR = Settings::getScreenRect();
    SDL_Rect rt = sR;
    const SDL_Color &fgr = traveler->getNation()->getForeground(), &bgr = traveler->getNation()->getBackground();
    int b = Settings::getTradeBorder(), r = Settings::getTradeRadius(), fS = Settings::getTradeFontSize();
    for (auto &g : traveler->getGoods())
        for (auto &m : g.getMaterials()) {
            Good dG(g.getId(), g.getAmount() * traveler->getPortion());
            boxes.push_back(m.button(true, g.getId(), g.getName(), g.getSplit(), rt, fgr, bgr, b, r, fS, game->getData(),
                                     [this, &dG] { traveler->deposit(dG); }));
        }
}

void Player::createManageButtons() {
    // Create buttons for managing businesses.
    //     SDL_Rect rt = {screenRect.w / 15, screenRect.h / 15, screenRect.w * 2 / 5, screenRect.h * 4 / 5};
}

void Player::createAttackButton() {
    // Create button for selecting traveler to fight.
    const SDL_Rect &sR = Settings::getScreenRect();
    SDL_Rect rt = {sR.w * 2 / 3, sR.h * 14 / 15, 0, 0};
    std::vector<std::string> tx = {"Cancel (F)ight"};
    boxes.push_back(std::make_unique<MenuButton>(rt, tx, Settings::getUIForeground(), Settings::getUIBackground(),
                                                 Settings::getSmallBoxBorder(), Settings::getSmallBoxRadius(),
                                                 Settings::getSmallBoxFontSize(), [this] { setState(traveling); }));
    boxes.back()->setKey(SDLK_f);
    // Create vector of names of attackable travelers.
    auto able = traveler->attackable();
    std::vector<std::string> names;
    names.reserve(able.size());
    std::transform(able.begin(), able.end(), std::back_inserter(names),
                   [](const std::shared_ptr<Traveler> t) { return t->getName(); });
    // Create attack button.
    rt = {sR.w / 4, sR.h / 4, sR.w / 2, sR.h / 2};
    boxes.push_back(std::make_unique<SelectButton>(rt, names, traveler->getNation()->getForeground(),
                                                   traveler->getNation()->getBackground(),
                                                   traveler->getNation()->getHighlight(), Settings::getBigBoxBorder(),
                                                   Settings::getBigBoxRadius(), Settings::getFightFontSize(), [this, able] {
                                                       int i = boxes.back()->getHighlightLine();
                                                       if (i > -1) {
                                                           traveler->attack(able[static_cast<size_t>(i)]);
                                                           setState(fighting);
                                                       } else
                                                           boxes.back()->setClicked(false);
                                                   }));
    pause = true;
    focus(0, Focusable::box);
}

void Player::createFightBoxes() {
    // Create buttons and text boxes for combat.
    auto target = traveler->getTarget().lock();
    if (not target) {
        setState(traveling);
        return;
    }
    const SDL_Rect &sR = Settings::getScreenRect();
    SDL_Rect rt = {sR.w / 2, sR.h / 4, 0, 0};
    std::vector<std::string> tx = {"Fighting " + target->getName() + "..."};
    boxes.push_back(std::make_unique<TextBox>(rt, tx, traveler->getNation()->getForeground(),
                                              traveler->getNation()->getBackground(), Settings::getBigBoxBorder(),
                                              Settings::getBigBoxRadius(), Settings::getFightFontSize()));
    rt = {sR.w / 21, sR.h / 4, sR.w * 5 / 21, sR.h / 2};
    tx = {};
    boxes.push_back(std::make_unique<TextBox>(rt, tx, traveler->getNation()->getForeground(),
                                              traveler->getNation()->getBackground(), Settings::getBigBoxBorder(),
                                              Settings::getBigBoxRadius(), Settings::getFightFontSize()));
    rt = {sR.w * 15 / 21, sR.h / 4, sR.w * 5 / 21, sR.h / 2};
    tx = {};
    boxes.push_back(std::make_unique<TextBox>(rt, tx, target->getNation()->getForeground(),
                                              target->getNation()->getBackground(), Settings::getBigBoxBorder(),
                                              Settings::getBigBoxRadius(), Settings::getFightFontSize()));
    rt = {sR.w / 3, sR.h / 3, sR.w / 3, sR.h / 3};
    tx = {"Fight", "Run", "Yield"};
    boxes.push_back(std::make_unique<SelectButton>(rt, tx, traveler->getNation()->getForeground(),
                                                   traveler->getNation()->getBackground(),
                                                   traveler->getNation()->getHighlight(), Settings::getBigBoxBorder(),
                                                   Settings::getBigBoxRadius(), Settings::getFightFontSize(), [this] {
                                                       int choice = boxes[kFightChoiceIndex]->getHighlightLine();
                                                       if (choice > -1) {
                                                           traveler->choose(static_cast<FightChoice>(choice));
                                                           pause = false;
                                                       }
                                                   }));
    focus(kFightChoiceIndex, Focusable::box);
}

void Player::updateFightBoxes() {
    // Create TextBox objects for fight user interface.
    std::vector<std::string> choiceText = boxes[0]->getText();
    switch (traveler->getChoice()) {
    case FightChoice::none:
        break;
    case FightChoice::fight:
        if (choiceText[0] == "Running...")
            choiceText[0] = "Running... Caught!";
        break;
    case FightChoice::run:
        choiceText[0] = "Running...";
        break;
    case FightChoice::yield:
        choiceText[0] = "Yielding...";
        break;
    }
    boxes[0]->setText(choiceText);
    std::vector<std::string> statusText(7);
    statusText[0] = traveler->getName() + "'s Status";
    for (size_t i = 1; i < statusText.size(); ++i)
        statusText[i] = game->getData().parts[i - 1] + ": " + game->getData().statuses[traveler->getPart(i - 1)];
    boxes[1]->setText(statusText);
    auto t = traveler->getTarget().lock();
    if (not t)
        return;
    statusText[0] = t->getName() + "'s Status";
    for (size_t i = 1; i < statusText.size(); ++i)
        statusText[i] = game->getData().parts[i - 1] + ": " + game->getData().statuses[t->getPart(i - 1)];
    boxes[2]->setText(statusText);
}

void Player::createLootButtons() {
    const SDL_Rect &sR = Settings::getScreenRect();
    auto target = traveler->getTarget().lock();
    if (not target) {
        setState(traveling);
        return;
    }
    SDL_Rect rt = {sR.w / 15, sR.h * 14 / 15, 0, 0};
    std::vector<std::string> tx = {"(D)one Looting"};
    boxes.push_back(std::make_unique<MenuButton>(rt, tx, Settings::getUIForeground(), Settings::getUIBackground(),
                                                 Settings::getSmallBoxBorder(), Settings::getSmallBoxRadius(),
                                                 Settings::getSmallBoxFontSize(), [this] {
                                                     traveler->loseTarget();
                                                     setState(traveling);
                                                 }));
    boxes.back()->setKey(SDLK_d);
    rt.x += sR.w / 5;
    tx = {"(L)oot All"};
    boxes.push_back(std::make_unique<MenuButton>(rt, tx, Settings::getUIForeground(), Settings::getUIBackground(),
                                                 Settings::getSmallBoxBorder(), Settings::getSmallBoxRadius(),
                                                 Settings::getSmallBoxFontSize(), [this, &t = *target] {
                                                     traveler->loot(t);
                                                     traveler->loseTarget();
                                                     setState(traveling);
                                                 }));
    boxes.back()->setKey(SDLK_l);
    int left = sR.w / Settings::getGoodButtonXDivisor(), right = sR.w / 2;
    int top = sR.h / Settings::getGoodButtonYDivisor();
    int dx = sR.w * 31 / Settings::getGoodButtonXDivisor(), dy = sR.h * 31 / Settings::getGoodButtonYDivisor();
    rt = {left, top, sR.w * 29 / Settings::getGoodButtonXDivisor(), sR.h * 29 / Settings::getGoodButtonYDivisor()};
    for (auto &g : traveler->getGoods())
        for (auto &m : g.getMaterials()) {
            if ((m.getAmount() >= 0.01 and g.getSplit()) or (m.getAmount() >= 1)) {
                boxes.push_back(m.button(true, g.getId(), g.getName(), g.getSplit(), rt,
                                         traveler->getNation()->getForeground(), traveler->getNation()->getBackground(),
                                         Settings::getTradeBorder(), Settings::getTradeRadius(), Settings::getTradeFontSize(), game->getData(),
                                         [this, &g, &m, &t = *target] {
                                             Good l(g.getId(), m.getAmount());
                                             l.addMaterial(Material(m.getId(), m.getAmount()));
                                             t.loot(l, *traveler);
                                             setState(looting);
                                         }));
                rt.x += dx;
                if (rt.x + rt.w >= right) {
                    rt.x = left;
                    rt.y += dy;
                }
            }
        }
    left = sR.w / 2 + sR.w / Settings::getGoodButtonXDivisor();
    right = sR.w - sR.w / Settings::getGoodButtonXDivisor();
    rt.x = left;
    rt.y = top;
    for (auto &g : target->getGoods())
        for (auto &m : g.getMaterials()) {
            if ((m.getAmount() >= 0.01 and g.getSplit()) or (m.getAmount() >= 1)) {
                boxes.push_back(m.button(true, g.getId(), g.getName(), g.getSplit(), rt,
                                         traveler->getNation()->getForeground(), traveler->getNation()->getBackground(),
                                         Settings::getTradeBorder(), Settings::getTradeRadius(), Settings::getTradeFontSize(), game->getData(),
                                         [this, &g, &m, &t = *target] {
                                             Good l(g.getId(), m.getAmount());
                                             l.addMaterial(Material(m.getId(), m.getAmount()));
                                             traveler->loot(l, t);
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

void Player::createDyingBoxes() {
    const SDL_Rect &sR = Settings::getScreenRect();
    SDL_Rect rt = {sR.w / 2, sR.h / 2, 0, 0};
    std::vector<std::string> tx = {traveler->getLogText().back(), "You have died."};
    boxes.push_back(std::make_unique<TextBox>(rt, tx, traveler->getNation()->getForeground(),
                                              traveler->getNation()->getBackground(), Settings::getBigBoxBorder(),
                                              Settings::getBigBoxRadius(), Settings::getFightFontSize()));
}

void Player::draw(SDL_Renderer *s) {
    if (traveler.get())
        traveler->draw(s);
    for (auto &b : boxes) {
        b->draw(s);
    }
}
