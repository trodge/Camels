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

Player::Player(Game &g)
    : game(g), printer(g.getPrinter()), screenRect(Settings::getScreenRect()),
      travelButtonsInfo({{{trading, {screenRect.w * 2 / 9, screenRect.h * 14 / 15, 0, 0}, "(T)rade", SDLK_t},
                          {storing, {screenRect.w / 3, screenRect.h * 14 / 15, 0, 0}, "(S)tore", SDLK_s},
                          {managing, {screenRect.w * 4 / 9, screenRect.h * 14 / 15, 0, 0}, "(M)anage", SDLK_m},
                          {equipping, {screenRect.w * 5 / 9, screenRect.h * 14 / 15, 0, 0}, "(E)quip", SDLK_e},
                          {hiring, {screenRect.w * 2 / 3, screenRect.h * 14 / 15, 0, 0}, "(H)ire", SDLK_h},
                          {attacking, {screenRect.w * 7 / 9, screenRect.h * 14 / 15, 0, 0}, "(A)ttack", SDLK_a},
                          {logging, {screenRect.w * 8 / 9, screenRect.h * 14 / 15, 0, 0}, "(L)og", SDLK_l}}}),
      stopButtonsInfo({{{trading, {screenRect.w * 2 / 9, screenRect.h * 14 / 15, 0, 0}, "Stop (T)rading", SDLK_t},
                        {storing, {screenRect.w / 3, screenRect.h * 14 / 15, 0, 0}, "Stop (S)toring", SDLK_s},
                        {managing, {screenRect.w * 4 / 9, screenRect.h * 14 / 15, 0, 0}, "Stop (M)anaging", SDLK_m},
                        {equipping, {screenRect.w * 5 / 9, screenRect.h * 14 / 15, 0, 0}, "Stop (E)quipping", SDLK_e},
                        {hiring, {screenRect.w * 2 / 3, screenRect.h * 14 / 15, 0, 0}, "Stop (H)iring", SDLK_h},
                        {attacking, {screenRect.w * 7 / 9, screenRect.h * 14 / 15, 0, 0}, "Cancel (A)ttack", SDLK_a},
                        {logging, {screenRect.w * 8 / 9, screenRect.h * 14 / 15, 0, 0}, "Close (L)og", SDLK_l}}}) {}

void Player::loadTraveler(const Save::Traveler *t, std::vector<Town> &ts) {
    // Load the traveler for the player from save file.
    traveler = std::make_shared<Traveler>(t, ts, game.getNations(), game.getData());
}

void Player::prepFocus(FocusGroup g, int &i, int &s, int &d, std::vector<TextBox *> &fcbls) {
    std::vector<Town *> neighbors;
    switch (g) {
    case box:
        i = focusBox;
        s = static_cast<int>(boxes.size());
        d = kBoxDBS;
        for (auto &b : boxes) fcbls.push_back(b.get());
        break;
    case neighbor:
        neighbors = traveler->getTown()->getNeighbors();
        i = static_cast<int>(
            std::find_if(neighbors.begin(), neighbors.end(),
                         [this](Town *t) { return t->getId() == static_cast<unsigned int>(focusTown + 1); }) -
            neighbors.begin());
        s = static_cast<int>(neighbors.size());
        d = kTownDBS;
        if (i == s) {
            // Currently focused town is last neighbor.
            auto &ts = game.getTowns();
            size_t fT = focusTown;
            if (fT < ts.size())
                ts[fT].getBox()->changeBorder(-kTownDBS);
            i = -1;
        }
        fcbls.reserve(neighbors.size());
        for (auto &ng : neighbors)
            fcbls.push_back(ng->getBox());
        break;
    case town:
        i = focusTown;
        fcbls = game.getTownBoxes();
        s = static_cast<int>(fcbls.size());
        d = kTownDBS;
        break;
    }
}

void Player::finishFocus(int f, FocusGroup g, const std::vector<TextBox *> &fcbls) {
    switch (g) {
    case box:
        focusBox = f;
        break;
    case neighbor:
        if (f > -1)
            focusTown = static_cast<int>(fcbls[static_cast<size_t>(f)]->getId() - 1);
        else
            focusTown = -1;
        break;
    case town:
        focusTown = f;
        break;
    }
}

void Player::focus(int f, FocusGroup g) {
    // Focus item f from group g.
    int i, s, d;
    std::vector<TextBox *> fcbls;
    prepFocus(g, i, s, d, fcbls);
    if (i > -1 && i < s) fcbls[static_cast<size_t>(i)]->changeBorder(-d);
    if (f > -1 && f < s) fcbls[static_cast<size_t>(f)]->changeBorder(d);
    finishFocus(f, g, fcbls);
}

void Player::focusPrev(FocusGroup g) {
    // Focus previous item from group g.
    int i, s, d;
    std::vector<TextBox *> fcbls;
    prepFocus(g, i, s, d, fcbls);
    if (i == -1) {
        i = s;
        while (i--)
            if (fcbls[static_cast<size_t>(i)]->getCanFocus()) break;
    } else {
        fcbls[static_cast<size_t>(i)]->changeBorder(-d);;
        int j = i;
        while (--i != j)
            if (i < 0)
                i = s;
            else if (fcbls[static_cast<size_t>(i)]->getCanFocus())
                break;
    }
    if (i > -1) {
        fcbls[static_cast<size_t>(i)]->changeBorder(d);
        finishFocus(i, g, fcbls);
    }
}

void Player::focusNext(FocusGroup g) {
    // Focus next item from group g.
    int i, s, d;
    std::vector<TextBox *> fcbls;
    prepFocus(g, i, s, d, fcbls);
    if (i == -1) {
        // Nothing was focused, find first focusable item.
        while (++i < s)
            if (fcbls[static_cast<size_t>(i)]->getCanFocus()) break;
        // No focusable item was found in group.
        return;
    } else {
        fcbls[static_cast<size_t>(i)]->changeBorder(-d);
        int j = i;
        while (++i != j)
            // Loop until we come back to the same item.
            if (i >= s)
                i = -1;
            else if (fcbls[static_cast<size_t>(i)]->getCanFocus())
                break;
    }
    fcbls[static_cast<size_t>(i)]->changeBorder(d);
    finishFocus(i, g, fcbls);
}

void Player::setState(UIState s) {
    // Change the UI state to s.
    boxes.clear();
    focusBox = -1;
    const SDL_Color &uIFgr = Settings::getUIForeground(), &uIBgr = Settings::getUIBackground();
    int bBB = Settings::getBigBoxBorder(), bBR = Settings::getBigBoxRadius(), bBFS = Settings::getBigBoxFontSize(),
        sBB = Settings::getSmallBoxBorder(), sBR = Settings::getSmallBoxRadius(),
        sBFS = Settings::getSmallBoxFontSize(), fFS = Settings::getFightFontSize();
    SDL_Rect rt;
    std::vector<std::string> tx(1);
    std::function<void()> f;
    fs::path path;
    std::vector<std::string> saves;
    std::array<ButtonInfo, 7>::iterator sBtnIIt;
    switch (s) {
    case starting:
        rt = {screenRect.w / 2, screenRect.h / 15, 0, 0};
        tx.back() = "Camels && Silk";
        boxes.push_back(std::make_unique<TextBox>(rt, tx, uIFgr, uIBgr, bBB, bBR, bBFS, printer));
        rt = {screenRect.w / 7, screenRect.h / 3, 0, 0};
        tx.back() = "(N)ew Game";
        boxes.push_back(std::make_unique<MenuButton>(rt, tx, uIFgr, uIBgr, bBB, bBR, bBFS, printer, [this] {
            game.newGame();
            setState(beginning);
        }));
        boxes.back()->setKey(SDLK_n);
        rt = {screenRect.w / 7, screenRect.h * 2 / 3, 0, 0};
        tx.back() = "(L)oad Game";
        boxes.push_back(std::make_unique<MenuButton>(rt, tx, uIFgr, uIBgr, bBB, bBR, bBFS, printer, [this] {
            boxes.back()->setClicked(false);
            toggleState(loading);
        }));
        boxes.back()->setKey(SDLK_l);
        break;
    case beginning:
        rt = {screenRect.w / 2, screenRect.h / 7, 0, 0};
        tx = {"Name", ""};
        boxes.push_back(std::make_unique<TextBox>(rt, tx, uIFgr, uIBgr, bBB, bBR, bBFS, printer));
        boxes.back()->toggleLock();
        for (auto &n : game.getNations()) {
            // Create a button for each nation to start in that nation.
            rt = {screenRect.w * (static_cast<int>(n.getId() - 1) % 3 * 2 + 1) / 6,
                  screenRect.h * (static_cast<int>(n.getId() - 1) / 3 + 2) / 7, 0, 0};
            boxes.push_back(std::make_unique<MenuButton>(rt, n.getNames(), n.getForeground(), n.getBackground(),
                                                         n.getId(), true, 3, bBR, bBFS, printer, [this, n] {
                                                             focusBox = -1;
                                                             unsigned int nId = n.getId(); // nation id
                                                             std::string name = boxes.front()->getText(1);
                                                             if (name.empty()) name = n.randomName();
                                                             // Create traveler object for player
                                                             traveler = game.createPlayerTraveler(nId, name);
                                                             show = true;
                                                             setState(traveling);
                                                         }));
        }
        break;
    case quitting:
        rt = {screenRect.w / 2, screenRect.h / 4, 0, 0};
        tx.back() = "Continue";
        boxes.push_back(std::make_unique<MenuButton>(rt, tx, uIFgr, uIBgr, bBB, bBR, bBFS, printer,
                                                     [this] { toggleState(quitting); }));
        rt = {screenRect.w / 2, screenRect.h * 3 / 4, 0, 0};
        if (traveler) {
            tx.back() = "Save && Quit";
            f = [this] {
                game.saveGame();
                stop = true;
            };
        } else {
            tx.back() = "Quit";
            f = [this] { stop = true; };
        }
        boxes.push_back(std::make_unique<MenuButton>(rt, tx, uIFgr, uIBgr, bBB, bBR, bBFS, printer, f));
        break;
    case loading:
        rt = {screenRect.w / 7, screenRect.h / 7, 0, 0};
        tx.back() = "(B)ack";
        boxes.push_back(std::make_unique<MenuButton>(rt, tx, uIFgr, uIBgr, bBB, bBR, bBFS, printer,
                                                     [this] { toggleState(loading); }));
        boxes.back()->setKey(SDLK_b);
        path = "save";
        for (auto &file : fs::directory_iterator(path)) saves.push_back(file.path().stem().string());
        rt = {screenRect.w / 2, screenRect.h / 15, 0, 0};
        tx.back() = "Load";
        boxes.push_back(std::make_unique<TextBox>(rt, tx, uIFgr, uIBgr, bBB, bBR, bBFS, printer));
        rt = {screenRect.w / 5, screenRect.h / 7, screenRect.w * 3 / 5, screenRect.h * 5 / 7};
        boxes.push_back(std::make_unique<SelectButton>(
            rt, saves, uIFgr, uIBgr, Settings::getUIHighlight(), bBB, bBR, bBFS, printer, [this, path] {
                game.loadGame((path / boxes.back()->getItem()).replace_extension("sav"));
                show = true;
                setState(traveling);
            }));
        break;
    case traveling:
        // Create go button.
        rt = {screenRect.w / 9, screenRect.h * 14 / 15, 0, 0};
        tx.back() = "(G)o";
        boxes.push_back(std::make_unique<MenuButton>(rt, tx, uIFgr, uIBgr, sBB, sBR, sBFS, printer, [this] {
            if (focusTown > -1) game.pickTown(traveler, static_cast<size_t>(focusTown));
            show = true;
            boxes.front()->setClicked(false);
        }));
        boxes.back()->setKey(SDLK_g);
        // Create trade, store, manage, equip, hire, attack, and log buttons
        for (auto &sRTK : travelButtonsInfo) {
            tx.back() = sRTK.text;
            boxes.push_back(std::make_unique<MenuButton>(sRTK.rect, tx, uIFgr, uIBgr, sBB, sBR, sBFS, printer,
                                                         [this, &sRTK] { setState(sRTK.state); }));
            boxes.back()->setKey(sRTK.key);
        }
        pause = false;
        break;
    case trading:
    case storing:
    case managing:
    case equipping:
    case hiring:
    case attacking:
    case logging:
        sBtnIIt = std::find_if(stopButtonsInfo.begin(), stopButtonsInfo.end(),
                               [s](const ButtonInfo &bI) { return bI.state == s; });
        if (sBtnIIt == stopButtonsInfo.end()) return;
        rt = sBtnIIt->rect;
        tx.back() = sBtnIIt->text;
        boxes.push_back(std::make_unique<MenuButton>(rt, tx, uIFgr, uIBgr, sBB, sBR, sBFS, printer,
                                                     [this] { setState(traveling); }));
        boxes.back()->setKey(sBtnIIt->key);
        switch (s) {
        case trading:
            rt.y -= boxes.back()->getRect().h * 3 / 2;
            tx.back() = "(C)omplete Trade";
            boxes.push_back(std::make_unique<MenuButton>(rt, tx, uIFgr, uIBgr, sBB, sBR, sBFS, printer, [this] {
                traveler->makeTrade();
                setState(trading);
            }));
            boxes.back()->setKey(SDLK_c);
            rt.x += screenRect.w / 6;
            tx.back() = traveler->portionString();
            portionBoxIndex = boxes.size();
            boxes.push_back(std::make_unique<TextBox>(rt, tx, uIFgr, uIBgr, sBB, sBR, sBFS, printer));
            boxes.back()->toggleLock();
            rt.y += boxes.back()->getRect().h * 3 / 2;
            tx.back() = "(S)et Portion";
            setPortionButtonIndex = boxes.size();
            boxes.push_back(std::make_unique<MenuButton>(rt, tx, uIFgr, uIBgr, sBB, sBR, sBFS, printer, [this] {
                double p;
                std::stringstream(boxes[portionBoxIndex]->getText().back()) >> p;
                traveler->setPortion(p);
                updatePortionBox();
                boxes[setPortionButtonIndex]->setClicked(false);
            }));
            boxes.back()->setKey(SDLK_s);
            offerButtonIndex = boxes.size();
            // Create buttons for trading goods.
            traveler->createTradeButtons(boxes, offerButtonIndex, requestButtonIndex, printer);
            break;
        case storing:
            storageButtonIndex = boxes.size();
            traveler->createStorageButtons(boxes, focusBox, storageButtonIndex, printer);
            break;
        case managing:
            traveler->createManageButtons(boxes, printer);
            break;
        case equipping:
            equipButtonIndex = boxes.size();
            traveler->createEquipButtons(boxes, focusBox, equipButtonIndex, printer);
            break;
        case hiring:
            break;
        case attacking:
            traveler->createAttackButton(
                boxes, [this] { setState(fighting); }, printer);
            pause = true;
            break;
        case logging:
            // Create log box.
            rt = {screenRect.w / 15, screenRect.h * 2 / 15, screenRect.w * 13 / 15, screenRect.h * 11 / 15};
            boxes.push_back(std::make_unique<ScrollBox>(rt, traveler->getLogText(), traveler->getNation()->getForeground(),
                                                        traveler->getNation()->getBackground(),
                                                        traveler->getNation()->getHighlight(), bBB, bBR, fFS, printer));
            boxes.back()->setHighlightLine(-1);
            break;
        default:
            break;
        }
        break;
    case fighting:
        traveler->createFightBoxes(boxes, pause, printer);
        break;
    case looting:
        rt = {screenRect.w / 15, screenRect.h * 14 / 15, 0, 0};
        tx.back() = "(D)one Looting";
        boxes.push_back(std::make_unique<MenuButton>(rt, tx, uIFgr, uIBgr, sBB, sBR, sBFS, printer, [this] {
            traveler->loseTarget();
            setState(traveling);
        }));
        boxes.back()->setKey(SDLK_d);
        rt.x += screenRect.w / 5;
        tx.back() = "(L)oot All";
        boxes.push_back(std::make_unique<MenuButton>(rt, tx, uIFgr, uIBgr, sBB, sBR, sBFS, printer,
                                                     [this, &tgt = *traveler->getTarget().lock()] {
                                                         traveler->loot(tgt);
                                                         traveler->loseTarget();
                                                         setState(traveling);
                                                     }));
        boxes.back()->setKey(SDLK_l);
        lootButtonIndex = boxes.size();
        traveler->createLootButtons(boxes, focusBox, lootButtonIndex, printer);
        pause = true;
        break;
    case dying:
        rt = {screenRect.w / 2, screenRect.h / 2, 0, 0};
        tx = {traveler->getLogText().back(), "You have died."};
        boxes.push_back(std::make_unique<TextBox>(rt, tx, traveler->getNation()->getForeground(),
                                                  traveler->getNation()->getBackground(), bBB, bBR, fFS, printer));
        break;
    }
    focus(0, box);
    state = s;
}

void Player::toggleState(UIState s) {
    // Set UI state to s if not already s, otherwise set to traveling.
    if (s == quitting && state == loading) s = loading;
    if (s == quitting || s == loading) {
        // Store or restore UI state.
        focus(-1, box);
        std::swap(boxes, storedBoxes);
        std::swap(pause, storedPause);
        std::swap(state, storedState);
        if (storedState == s)
            // Stored boxes have been restored, no need to create new boxes.
            return;
    }
    if (state == s && !(s == quitting || s == loading))
        setState(traveling);
    else if (!(state == fighting || state == dying) || s == quitting)
        setState(s);
}

void Player::handleKey(const SDL_KeyboardEvent &k) {
    SDL_Keymod mod = SDL_GetModState();
    if ((mod & KMOD_CTRL) && (mod & KMOD_SHIFT) && (mod & KMOD_ALT))
        modMultiplier = 10000;
    else if ((mod & KMOD_CTRL) && (mod & KMOD_ALT))
        modMultiplier = 0.001;
    else if ((mod & KMOD_SHIFT) && (mod & KMOD_ALT))
        modMultiplier = 0.01;
    else if ((mod & KMOD_CTRL) && (mod & KMOD_SHIFT))
        modMultiplier = 1000;
    else if (mod & KMOD_ALT)
        modMultiplier = 0.1;
    else if (mod & KMOD_SHIFT)
        modMultiplier = 10;
    else if (mod & KMOD_CTRL)
        modMultiplier = 100;
    else
        modMultiplier = 1;
    if (k.state == SDL_PRESSED) {
        // Event is SDL_KEYDOWN
        if (!boxes.empty()) {
            auto keyedBox = std::find_if(boxes.begin(), boxes.end(),
                                         [&k](const std::unique_ptr<TextBox> &t) { return k.keysym.sym == t->getKey(); });
            if (keyedBox != boxes.end()) {
                // A box's key was pressed.
                focus(static_cast<int>(std::distance(boxes.begin(), keyedBox)), box);
                (*keyedBox)->handleKey(k);
            }
        }
        if (!(focusBox > -1 && boxes[static_cast<size_t>(focusBox)]->keyCaptured(k))) {
            if (state != quitting) {
                if (traveler) {
                    switch (state) {
                    case traveling:
                        switch (k.keysym.sym) {
                        case SDLK_LEFT:
                        case SDLK_RIGHT:
                        case SDLK_UP:
                        case SDLK_DOWN:
                            scrollKeys.insert(k.keysym.sym);
                            show = false;
                            break;
                        case SDLK_n:
                            if (mod & KMOD_SHIFT)
                                focusPrev(neighbor);
                            else
                                focusNext(neighbor);
                            break;
                        }
                        break;
                    case trading:
                        switch (k.keysym.sym) {
                        case SDLK_LEFT:
                            focusPrev(box);
                            break;
                        case SDLK_RIGHT:
                            focusNext(box);
                            break;
                        case SDLK_UP:
                            for (int i = 0; i < 7; ++i) focusPrev(box);
                            break;
                        case SDLK_DOWN:
                            for (int i = 0; i < 7; ++i) focusNext(box);
                            break;
                        case SDLK_COMMA:
                            traveler->changePortion(-0.1);
                            updatePortionBox();
                            break;
                        case SDLK_PERIOD:
                            traveler->changePortion(0.1);
                            updatePortionBox();
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
                if (mod & KMOD_SHIFT)
                    focusPrev(box);
                else
                    focusNext(box);
                break;
            }
        } else
            boxes[static_cast<size_t>(focusBox)]->handleKey(k);
    } else {
        // Event is SDL_KEYUP.
        if (!boxes.empty()) {
            // Find a box which handles this key.
            auto keyedBox = std::find_if(boxes.begin(), boxes.end(),
                                         [&k](const std::unique_ptr<TextBox> &t) { return k.keysym.sym == t->getKey(); });
            if (keyedBox != boxes.end())
                // A box's key was released.
                (*keyedBox)->handleKey(k);
            if (!(focusBox > -1 && boxes[static_cast<size_t>(focusBox)]->keyCaptured(k)))
                // Key is not used by currently focused box.
                switch (k.keysym.sym) {
                case SDLK_LEFT:
                case SDLK_RIGHT:
                case SDLK_UP:
                case SDLK_DOWN:
                    scrollKeys.erase(k.keysym.sym);
                    break;
                }
            else
                boxes[static_cast<size_t>(focusBox)]->handleKey(k);
        }
    }
}

void Player::handleTextInput(const SDL_TextInputEvent &t) {
    if (focusBox > -1) boxes[static_cast<size_t>(focusBox)]->handleTextInput(t);
}

void Player::handleClick(const SDL_MouseButtonEvent &b) {
    if (state == traveling) {
        std::vector<TextBox *> fcbls = game.getTownBoxes();
        auto clickedTown =
            std::find_if(fcbls.begin(), fcbls.end(), [&b](const TextBox *t) { return t->clickCaptured(b); });
        if (clickedTown != fcbls.end())
            focus(static_cast<int>(std::distance(fcbls.begin(), clickedTown)), town);
    }
    if (!boxes.empty()) {
        auto clickedBox = std::find_if(boxes.begin(), boxes.end(), [&b](const std::unique_ptr<TextBox> &t) {
            return t->getCanFocus() && t->clickCaptured(b);
        });
        if (clickedBox != boxes.end()) {
            int cI = static_cast<int>(std::distance(boxes.begin(), clickedBox));
            if (cI != focusBox) focus(cI, box);
            (*clickedBox)->handleClick(b);
        } else
            focus(-1, box);
    }
}

void Player::handleEvent(const SDL_Event &e) {
    switch (e.type) {
    case SDL_KEYDOWN:
    case SDL_KEYUP:
        handleKey(e.key);
        break;
    case SDL_TEXTINPUT:
        handleTextInput(e.text);
        break;
    case SDL_MOUSEBUTTONDOWN:
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
    bool target = t && t->getTarget().lock();
    switch (state) {
    case traveling:
        if (target) {
            pause = true;
            setState(fighting);
        }
        break;
    case trading:
        traveler->updateTradeButtons(boxes, offerButtonIndex, requestButtonIndex);
        break;
    case fighting:
        if (!traveler->alive()) {
            setState(dying);
            break;
        } else if (!target) {
            setState(logging);
            break;
        } else if (traveler->fightWon()) {
            setState(looting);
            break;
        }
        traveler->updateFightBoxes(boxes);
        break;
    default:
        break;
    }
    int scrollX = 0, scrollY = 0, sS = Settings::getScroll();
    if (show) {
        const SDL_Rect &mR = game.getMapRect();
        if (traveler->getPX() < int(mR.w * kShowPlayerPadding)) scrollX = -sS;
        if (traveler->getPY() < int(mR.h * kShowPlayerPadding)) scrollY = -sS;
        if (traveler->getPX() > int(mR.w * (1 - kShowPlayerPadding))) scrollX = sS;
        if (traveler->getPY() > int(mR.h * (1 - kShowPlayerPadding))) scrollY = sS;
    }
    if (!(scrollKeys.empty())) {
        auto upIt = scrollKeys.find(SDLK_UP), dnIt = scrollKeys.find(SDLK_DOWN), lfIt = scrollKeys.find(SDLK_LEFT),
             rtIt = scrollKeys.find(SDLK_RIGHT);
        scrollX -= (lfIt != scrollKeys.end()) * static_cast<int>(static_cast<double>(sS) * modMultiplier);
        scrollX += (rtIt != scrollKeys.end()) * static_cast<int>(static_cast<double>(sS) * modMultiplier);
        scrollY -= (upIt != scrollKeys.end()) * static_cast<int>(static_cast<double>(sS) * modMultiplier);
        scrollY += (dnIt != scrollKeys.end()) * static_cast<int>(static_cast<double>(sS) * modMultiplier);
    }
    if (scrollX || scrollY) game.moveView(scrollX, scrollY);
    if (traveler.get() && !pause) traveler->update(e);
}

void Player::updatePortionBox() { boxes[portionBoxIndex]->setText(0, traveler->portionString()); }

void Player::draw(SDL_Renderer *s) {
    if (traveler.get()) traveler->draw(s);
    for (auto &b : boxes) { b->draw(s); }
}
