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

Player::Player(Game &g) : game(g), printer(g.getPrinter()) {
    const auto &sR = Settings::getScreenRect();
    int bBB = Settings::getBigBoxBorder(), bBR = Settings::getBigBoxRadius(), bBFS = Settings::getBigBoxFontSize(),
        sBB = Settings::getSmallBoxBorder(), sBR = Settings::getSmallBoxRadius(), sBFS = Settings::getSmallBoxFontSize();
    uIStates = {{UIState::starting,
                 {{Settings::getBoxInfo(true, {sR.w / 2, sR.h / 15, 0, 0}, {"Camels and Silk"}),
                   Settings::getBoxInfo(true, {sR.w / 7, sR.h / 3, 0, 0}, {"(N)ew Game"}, SDLK_n,
                                        [this, sR, bBB, bBR, bBFS](MenuButton *) {
                                            game.newGame();
                                            auto &nations = game.getNations();
                                            auto &beginningBoxes = uIStates.at(UIState::beginning).boxesInfo;
                                            for (auto &nt : nations)
                                                // Create a button for each nation to start in that nation.
                                                beginningBoxes.push_back(
                                                    {.rect = {sR.w * (static_cast<int>(nt.getId() - 1) % 3 * 2 + 1) / 6,
                                                              sR.h * (static_cast<int>(nt.getId() - 1) / 3 + 2) / 7, 0, 0},
                                                     .text = nt.getNames(),
                                                     .foreground = nt.getForeground(),
                                                     .background = nt.getBackground(),
                                                     .id = nt.getId(),
                                                     .isNation = true,
                                                     .border = bBB,
                                                     .radius = bBR,
                                                     .fontSize = bBFS,
                                                     .onClick = [this, nt](MenuButton *) {
                                                         unsigned int nId = nt.getId(); // nation id
                                                         std::string name = pagers[0].getVisible(0)->getText(1);
                                                         if (name.empty()) name = nt.randomName();
                                                         // Create traveler object for player
                                                         traveler = game.createPlayerTraveler(nId, name);
                                                         traveler->goods[55].create(0.75);
                                                         traveler->goods[59].create(2);
                                                         show = true;
                                                         focusBox = -1;
                                                         setState(UIState::traveling);
                                                     }});
                                            setState(UIState::beginning);
                                        }),
                   Settings::getBoxInfo(true, {sR.w / 7, sR.h * 2 / 3, 0, 0}, {"(L)oad Game"}, SDLK_l,
                                        [this](MenuButton *) { setState(UIState::loading); })}}},
                {UIState::beginning,
                 {{Settings::getBoxInfo(true, {sR.w / 2, sR.h / 7, 0, 0}, {"Name", ""})},
                  [this] {
                      // Allow typing in name box.
                      pagers[0].toggleLock(0);
                  }}},
                {UIState::quitting,
                 {{Settings::getBoxInfo(true, {sR.w / 2, sR.h * 3 / 4, 0, 0}, {""}, SDLK_q,
                                        [this](MenuButton *) {
                                            if (traveler) game.saveGame();
                                            stop = true;
                                        }),
                   Settings::getBoxInfo(true, {sR.w / 2, sR.h / 4, 0, 0}, {"Continue"}, SDLK_ESCAPE,
                                        [this](MenuButton *) {
                                            pause = storedPause;
                                            setState(storedState);
                                        })},
                  [this] {
                      storedState = state;
                      storedPause = pause;
                      pagers[0].getVisible(0)->setText(0, traveler ? "Save and Quit" : "Quit");
                  }}},
                {UIState::loading,
                 {{Settings::getBoxInfo(true, {sR.w / 7, sR.h / 7, 0, 0}, {"(B)ack"}, SDLK_b,
                                        [this](MenuButton *) { setState(UIState::starting); })}}},
                {UIState::traveling,
                 {{Settings::getBoxInfo(false, {sR.w / 9, sR.h * 30 / 31, 0, 0}, {"(G)o"}, SDLK_g,
                                        [this](MenuButton *btn) {
                                            if (focusTown > -1)
                                                game.pickTown(traveler.get(), static_cast<size_t>(focusTown));
                                            show = true;
                                            btn->setClicked(false);
                                        }),
                   Settings::getBoxInfo(false, {sR.w * 2 / 9, sR.h * 30 / 31, 0, 0}, {"(T)rade"}, SDLK_t,
                                        [this](MenuButton *) { setState(UIState::trading); }),
                   Settings::getBoxInfo(false, {sR.w / 3, sR.h * 30 / 31, 0, 0}, {"(S)tore"}, SDLK_s,
                                        [this](MenuButton *) { setState(UIState::storing); }),
                   Settings::getBoxInfo(false, {sR.w * 4 / 9, sR.h * 30 / 31, 0, 0}, {"(B)uild"}, SDLK_b,
                                        [this](MenuButton *) { setState(UIState::building); }),
                   Settings::getBoxInfo(false, {sR.w * 5 / 9, sR.h * 30 / 31, 0, 0}, {"(E)quip"}, SDLK_e,
                                        [this](MenuButton *) { setState(UIState::equipping); }),
                   Settings::getBoxInfo(false, {sR.w * 2 / 3, sR.h * 30 / 31, 0, 0}, {"(M)anage"}, SDLK_m,
                                        [this](MenuButton *) { setState(UIState::managing); }),
                   Settings::getBoxInfo(false, {sR.w * 7 / 9, sR.h * 30 / 31, 0, 0}, {"(A)ttack"}, SDLK_a,
                                        [this](MenuButton *) { setState(UIState::attacking); }),
                   Settings::getBoxInfo(false, {sR.w * 8 / 9, sR.h * 30 / 31, 0, 0}, {"(L)og"}, SDLK_l,
                                        [this](MenuButton *) { setState(UIState::logging); })},
                  [this] { pause = false; }}},
                {UIState::trading,
                 {{Settings::getBoxInfo(false, {sR.w * 2 / 9, sR.h * 30 / 31, 0, 0}, {"Stop (T)rading"}, SDLK_t,
                                        [this](MenuButton *) { setState(UIState::traveling); }),
                   Settings::getBoxInfo(false, {sR.w * 2 / 9, sR.h * 28 / 31, 0, 0}, {"(C)omplete Trade"}, SDLK_c,
                                        [this](MenuButton *) {
                                            traveler->makeTrade();
                                            setState(UIState::trading);
                                        })

                  },
                  [this] {
                      traveler->createTradeButtons(pagers, printer);
                      createBoxes(UIState::trading);
                  },
                  3}},
                {UIState::storing,
                 {{Settings::getBoxInfo(false, {sR.w / 3, sR.h * 30 / 31, 0, 0}, {"Stop (S)toring"}, SDLK_s,
                                        [this](MenuButton *) { setState(UIState::traveling); })},
                  [this] {
                      traveler->createStorageButtons(pagers, focusBox, printer);
                      createBoxes(UIState::storing);
                  },
                  3}},
                {UIState::building,
                 {{Settings::getBoxInfo(false, {sR.w * 4 / 9, sR.h * 30 / 31, 0, 0}, {"Stop (B)uilding"}, SDLK_b,
                                        [this](MenuButton *) { setState(UIState::traveling); })},
                  [this] {
                      traveler->createBuildButtons(pagers, focusBox, printer);
                      createBoxes(UIState::building);
                  },
                  3}},
                {UIState::equipping,
                 {{Settings::getBoxInfo(false, {sR.w * 5 / 9, sR.h * 30 / 31, 0, 0}, {"Stop (E)quipping"}, SDLK_e,
                                        [this](MenuButton *) { setState(UIState::traveling); })},
                  [this] { traveler->createEquipButtons(pagers, focusBox, printer); },
                  2}},
                {UIState::managing,
                 {{Settings::getBoxInfo(false, {sR.w * 2 / 3, sR.h * 30 / 31, 0, 0}, {"Stop (M)anaging"}, SDLK_m,
                                        [this](MenuButton *) { setState(UIState::traveling); })}}},
                {UIState::attacking,
                 {{Settings::getBoxInfo(false, {sR.w * 7 / 9, sR.h * 30 / 31, 0, 0}, {"Cancel (A)ttack"}, SDLK_a,
                                        [this](MenuButton *) { setState(UIState::traveling); })},
                  [this] {
                      pause = true;
                      traveler->createAttackButton(
                          pagers[0], [this] { setState(UIState::fighting); }, printer);
                  }}},
                {UIState::logging,
                 {{Settings::getBoxInfo(false, {sR.w * 8 / 9, sR.h * 30 / 31, 0, 0}, {"Close (L)og"}, SDLK_l,
                                        [this](MenuButton *) { setState(UIState::traveling); })},
                  [this, sR, bBB, bBR, sBFS] {
                      // Create log box.
                      pagers[0].addBox(std::make_unique<ScrollBox>(BoxInfo{{sR.w / 15, sR.h * 2 / 15, sR.w * 28 / 31, sR.h * 11 / 15},
                                                                           traveler->getLogText(),
                                                                           traveler->getNation()->getForeground(),
                                                                           traveler->getNation()->getBackground(),
                                                                           traveler->getNation()->getHighlight(),
                                                                           0,
                                                                           false,
                                                                           bBB,
                                                                           bBR,
                                                                           sBFS,
                                                                           {},
                                                                           SDLK_UNKNOWN,
                                                                           nullptr,
                                                                           true},
                                                                   printer));
                  }}},
                {UIState::fighting, {{}, [this] { traveler->createFightBoxes(pagers[0], pause, printer); }}},
                {UIState::looting,
                 {{Settings::getBoxInfo(false, {sR.w / 15, sR.h * 30 / 31, 0, 0}, {"Stop (L)ooting"}, SDLK_l,
                                        [this](MenuButton *) {
                                            traveler->loseTarget();
                                            setState(UIState::traveling);
                                        }),
                   Settings::getBoxInfo(false, {sR.w * 4 / 15, sR.h * 30 / 31, 0, 0}, {"Loot (A)ll"}, SDLK_a,
                                        [this](MenuButton *) {
                                            auto tgt = traveler->getTarget();
                                            traveler->loot(*tgt);
                                            traveler->loseTarget();
                                            setState(UIState::traveling);
                                        })},
                  [this] {
                      traveler->createLootButtons(pagers, focusBox, printer);
                      createBoxes(UIState::looting);
                  },
                  3}},
                {UIState::dying, {{}, [this, &sR, bBB, bBR, bBFS] {
                                      pagers[0].addBox(std::make_unique<TextBox>(
                                          BoxInfo{.rect = {sR.w / 2, sR.h / 2, 0, 0},
                                                  .text = {traveler->getLogText().back(), "You have died."},
                                                  .foreground = traveler->getNation()->getForeground(),
                                                  .background = traveler->getNation()->getBackground(),
                                                  .highlight = traveler->getNation()->getHighlight(),
                                                  .border = bBB,
                                                  .radius = bBR,
                                                  .fontSize = bBFS},
                                          printer));
                                  }}}};

    fs::path path{"save"};
    std::vector<std::string> saves;
    for (auto &file : fs::directory_iterator{path}) saves.push_back(file.path().stem().string());
    uIStates.at(UIState::loading)
        .boxesInfo.push_back(Settings::getBoxInfo(
            true, {sR.w / 5, sR.h / 7, sR.w * 3 / 5, sR.h * 5 / 7}, saves, SDLK_l,
            [this, &path](MenuButton *btn) {
                game.loadGame((path / btn->getItem()).replace_extension("sav"));
                show = true;
                setState(UIState::traveling);
            },
            true));
}

void Player::loadTraveler(const Save::Traveler *t, std::vector<Town> &ts) {
    // Load the traveler for the player from save file.
    traveler = std::make_unique<Traveler>(t, ts, game.getNations(), game.getData());
}

void Player::prepFocus(FocusGroup g, int &i, int &s, std::vector<TextBox *> &fcbls) {
    std::vector<Town *> neighbors;
    switch (g) {
    case box:
        i = focusBox;
        for (auto &pg : pagers) {
            // Put all visible boxes from all pages in focusables vector.
            auto bxs = pg.getVisible();
            fcbls.insert(end(fcbls), begin(bxs), end(bxs));
        }
        s = static_cast<int>(fcbls.size());
        break;
    case neighbor:
        neighbors = traveler->getTown()->getNeighbors();
        i = static_cast<int>(
            std::find_if(begin(neighbors), end(neighbors),
                         [this](Town *t) { return t->getId() == static_cast<unsigned int>(focusTown + 1); }) -
            begin(neighbors));
        s = static_cast<int>(neighbors.size());
        if (i == s) {
            // Currently focused town is last neighbor.
            auto &ts = game.getTowns();
            size_t fT = focusTown;
            if (fT < ts.size()) ts[fT].getBox()->toggleFocus();
            i = -1;
        }
        fcbls.reserve(neighbors.size());
        for (auto &ng : neighbors) fcbls.push_back(ng->getBox());
        break;
    case town:
        i = focusTown;
        fcbls = game.getTownBoxes();
        s = static_cast<int>(fcbls.size());
        break;
    }
}

void Player::finishFocus(int f, FocusGroup g, const std::vector<TextBox *> &fcbls) {
    switch (g) {
    case box:
        focusBox = f;
        // Set current pager to pager that contains focus box.
        for (currentPager = begin(pagers); currentPager != end(pagers); ++currentPager) {
            int vC = currentPager->visibleCount();
            if (f >= vC)
                f -= vC;
            else
                break;
        }
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
    int i, s;
    std::vector<TextBox *> fcbls;
    prepFocus(g, i, s, fcbls);
    if (i == f) return;
    if (i > -1 && i < s) fcbls[static_cast<size_t>(i)]->toggleFocus();
    if (f > -1 && f < s) fcbls[static_cast<size_t>(f)]->toggleFocus();
    finishFocus(f, g, fcbls);
}

void Player::focusPrev(FocusGroup g) {
    // Focus previous item from group g.
    int i, s;
    std::vector<TextBox *> fcbls;
    prepFocus(g, i, s, fcbls);
    if (i == -1) {
        i = s;
        while (i--)
            if (fcbls[static_cast<size_t>(i)]->getCanFocus()) break;
        if (i < 0)
            // No focusable item found in group.
            return;
    } else {
        fcbls[static_cast<size_t>(i)]->toggleFocus();
        int j = i;
        while (--i != j)
            if (i < 0)
                i = s;
            else if (fcbls[static_cast<size_t>(i)]->getCanFocus())
                break;
    }
    fcbls[static_cast<size_t>(i)]->toggleFocus();
    finishFocus(i, g, fcbls);
}

void Player::focusNext(FocusGroup g) {
    // Focus next item from group g.
    int i, s;
    std::vector<TextBox *> fcbls;
    prepFocus(g, i, s, fcbls);
    if (i == -1) {
        // Nothing was focused, find first focusable item.
        while (++i < s)
            if (fcbls[static_cast<size_t>(i)]->getCanFocus()) break;
        if (i == s) {
            i = -1;
            // No focusable item was found in group.
            return;
        }
    } else {
        fcbls[static_cast<size_t>(i)]->toggleFocus();
        int j = i;
        while (++i != j)
            // Loop until we come back to the same item.
            if (i == s)
                i = -1;
            else if (fcbls[static_cast<size_t>(i)]->getCanFocus())
                break;
    }
    fcbls[static_cast<size_t>(i)]->toggleFocus();
    finishFocus(i, g, fcbls);
}

void Player::createBoxes(UIState::State s) {
    // Creates boxes shared by trading, storing, managing, and looting states.
    const auto &sR = Settings::getScreenRect();
    // Create portion box and set portion button.
    auto portionBox = std::make_unique<TextBox>(Settings::getBoxInfo(false, {sR.w / 9, sR.h * 28 / 31, 0, 0}, {""}), printer);
    portionBox->toggleLock();
    traveler->updatePortionBox(portionBox.get());
    auto setPortionButton =
        std::make_unique<MenuButton>(Settings::getBoxInfo(false, {sR.w / 9, sR.h * 30 / 31, 0, 0}, {"(S)et Portion"}, SDLK_s,
                                                          [this, pB = portionBox.get()](MenuButton *btn) {
                                                              double p;
                                                              std::stringstream{pB->getText(0)} >> p;
                                                              traveler->setPortion(p);
                                                              traveler->updatePortionBox(pB);
                                                              btn->setClicked(false);
                                                          }),
                                     printer);
    pagers[0].addBox(std::move(portionBox));
    pagers[0].addBox(std::move(setPortionButton));
    // Create labels.
    auto typeText = s == UIState::building ? "Businesses" : "Goods";
    pagers[0].addBox(std::make_unique<TextBox>(
        Settings::getBoxInfo(false, {sR.w / 4, sR.h / 31, 0, 0}, {traveler->getName() + "'s " + typeText}), printer));
    pagers[0].addBox(std::make_unique<TextBox>(
        Settings::getBoxInfo(false, {sR.w * 3 / 4, sR.h / 31, 0, 0},
                             {s == UIState::storing ?
                                  "Goods in " + traveler->toTown->getName() :
                                  (s == UIState::looting ? traveler->target->getName() : traveler->toTown->getName()) + "'s " + typeText}),
        printer));
    // Create page switching buttons.
    std::array<SDL_Keycode, 4> keys{SDLK_MINUS, SDLK_EQUALS, SDLK_LEFTBRACKET, SDLK_RIGHTBRACKET};
    auto kyIt = keys.begin();
    for (auto pgIt = begin(pagers) + 1; pgIt != end(pagers); ++pgIt) {
        if (pgIt->pageCount() < 2) {
            // Don't create buttons for pagers with only one page.
            kyIt += 2;
            continue;
        }
        auto &bnds = pgIt->getBounds();
        pagers[0].addBox(std::make_unique<MenuButton>(
            Settings::getBoxInfo(false, {bnds.x + bnds.w / 6, sR.h * 30 / 31, 0, 0}, {"Previous Page"}, *kyIt++,
                                 [pgIt](MenuButton *btn) {
                                     pgIt->recedePage();
                                     btn->setClicked(false);
                                 }),
            printer));
        pagers[0].addBox(std::make_unique<MenuButton>(
            Settings::getBoxInfo(false, {bnds.x + bnds.w * 5 / 6, sR.h * 30 / 31, 0, 0}, {"Next Page"}, *kyIt++,
                                 [pgIt](MenuButton *btn) {
                                     pgIt->advancePage();
                                     btn->setClicked(false);
                                 }),
            printer));
    }
}

void Player::setState(UIState::State s) {
    // Change the UI state to s.
    UIState newState = uIStates.at(s);
    pagers.clear();
    pagers.resize(newState.pagerCount);
    currentPager = begin(pagers);
    focusBox = -1;
    fs::path path;
    std::vector<std::string> saves;
    // Create boxes for new state.
    for (auto &bI : newState.boxesInfo)
        if (bI.onClick && bI.scrolls)
            pagers[0].addBox(std::make_unique<SelectButton>(bI, printer));
        else if (bI.onClick)
            pagers[0].addBox(std::make_unique<MenuButton>(bI, printer));
        else if (bI.scrolls)
            pagers[0].addBox(std::make_unique<ScrollBox>(bI, printer));
        else
            pagers[0].addBox(std::make_unique<TextBox>(bI, printer));
    if (newState.onChange) newState.onChange();
    focusNext(FocusGroup::box);
    state = s;
}

void Player::handleKey(const SDL_KeyboardEvent &k) {
    SDL_Keymod mod = SDL_GetModState();
    if ((mod & KMOD_CTRL) && (mod & KMOD_ALT) && (mod & KMOD_SHIFT))
        modMultiplier = 10000.;
    else if ((mod & KMOD_CTRL) && (mod & KMOD_ALT))
        modMultiplier = 0.001;
    else if ((mod & KMOD_CTRL) && (mod & KMOD_SHIFT))
        modMultiplier = 1000.;
    else if ((mod & KMOD_ALT) && (mod & KMOD_SHIFT))
        modMultiplier = 0.01;
    else if (mod & KMOD_CTRL)
        modMultiplier = 100.;
    else if (mod & KMOD_ALT)
        modMultiplier = 0.1;
    else if (mod & KMOD_SHIFT)
        modMultiplier = 10.;
    else
        modMultiplier = 1.;
    int keyedIndex = -1, indexOffset = 0;
    for (auto &pgr : pagers) {
        keyedIndex = pgr.getKeyedIndex(k);
        if (keyedIndex > -1) {
            // Something on this page was keyed.
            pgr.getVisible(keyedIndex)->handleKey(k);
            break;
        }
        indexOffset += pgr.visibleCount();
    }
    if (k.state == SDL_PRESSED) {
        // Key was pressed down.
        if (keyedIndex > -1)
            // A box's key was pressed.
            focus(keyedIndex + indexOffset, FocusGroup::box);
        if (state != UIState::quitting) {
            if (traveler) {
                int columnCount = state == UIState::building ?
                                      Settings::getBusinessButtonXDivisor() / Settings::getBusinessButtonSpaceMultiplier() :
                                      Settings::getGoodButtonXDivisor() / Settings::getGoodButtonSpaceMultiplier();
                switch (state) {
                case UIState::beginning:
                case UIState::traveling:
                    switch (k.keysym.sym) {
                    case SDLK_LEFT:
                        scroll.insert(left);
                        show = false;
                        break;
                    case SDLK_RIGHT:
                        scroll.insert(right);
                        show = false;
                        break;
                    case SDLK_UP:
                        scroll.insert(up);
                        show = false;
                        break;
                    case SDLK_DOWN:
                        scroll.insert(down);
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
                case UIState::trading:
                    if (developer) switch (k.keysym.sym) {
                        case SDLK_x:
                            traveler->toTown->toggleMaxGoods();
                            break;
                        case SDLK_c:
                            traveler->toTown->resetGoods();
                            break;
                        case SDLK_v:
                            traveler->adjustDemand(pagers[2], -modMultiplier);
                            break;
                        case SDLK_b:
                            traveler->adjustDemand(pagers[2], modMultiplier);
                            break;
                        case SDLK_n:
                            traveler->adjustAreas(pagers[2], -modMultiplier);
                            break;
                        case SDLK_m:
                            traveler->adjustAreas(pagers[2], modMultiplier);
                            break;
                        case SDLK_r:
                            game.generateRoutes();
                            break;
                        case SDLK_s:
                            game.saveData();
                            break;
                        }
                case UIState::storing:
                case UIState::building:
                    switch (k.keysym.sym) {
                    case SDLK_LEFT:
                        focusPrev(box);
                        break;
                    case SDLK_RIGHT:
                        focusNext(box);
                        break;
                    case SDLK_UP:
                        for (int i = 0; i < columnCount; ++i) focusPrev(box);
                        break;
                    case SDLK_DOWN:
                        for (int i = 0; i < columnCount; ++i) focusNext(box);
                        break;
                    case SDLK_COMMA:
                        traveler->changePortion(-0.1);
                        traveler->updatePortionBox(pagers[0].getVisible(kPortionBoxIndex));
                        break;
                    case SDLK_PERIOD:
                        traveler->changePortion(0.1);
                        traveler->updatePortionBox(pagers[0].getVisible(kPortionBoxIndex));
                        break;
                    }
                    break;
                default:
                    break;
                }
            }
            // Keyboard shortcuts for all non-quitting states
            switch (k.keysym.sym) {
            case SDLK_ESCAPE:
                storedState = state;
                setState(UIState::quitting);
                break;
            }
        }
        // Keyboard shortcuts for all states.
        switch (k.keysym.sym) {
        case SDLK_BACKQUOTE:
            developer = !developer;
            break;
        case SDLK_TAB:
            if (mod & KMOD_SHIFT)
                focusPrev(box);
            else
                focusNext(box);
            break;
        }
    } else {
        // Key was lifted up.
        switch (k.keysym.sym) {
        case SDLK_LEFT:
            scroll.erase(left);
            break;
        case SDLK_RIGHT:
            scroll.erase(right);
            break;
        case SDLK_UP:
            scroll.erase(up);
            break;
        case SDLK_DOWN:
            scroll.erase(down);
            break;
        }
    }
}

void Player::handleTextInput(const SDL_TextInputEvent &t) {
    if (focusBox > -1) {
        int indexOffset = 0;
        for (auto &pgr : pagers) {
            int vC = pgr.visibleCount();
            if (focusBox - indexOffset < vC) {
                // Focus box is in this pager.
                pgr.getVisible(focusBox - indexOffset)->handleTextInput(t);
                break;
            }
            indexOffset += vC;
        }
    }
}

void Player::handleClick(const SDL_MouseButtonEvent &b) {
    if (state == UIState::traveling) {
        std::vector<TextBox *> fcbls = game.getTownBoxes();
        auto clickedTown = std::find_if(begin(fcbls), end(fcbls), [&b](const TextBox *t) { return t->clickCaptured(b); });
        if (clickedTown != end(fcbls)) focus(static_cast<int>(std::distance(begin(fcbls), clickedTown)), town);
    }
    int clickedIndex = -1, indexOffset = 0;
    for (auto &pgr : pagers) {
        clickedIndex = pgr.getClickedIndex(b);
        if (clickedIndex > -1) {
            pgr.getVisible(clickedIndex)->handleClick(b);
            break;
        }
        indexOffset += pgr.visibleCount();
    }
    if (b.state == SDL_PRESSED)
        if (clickedIndex < 0)
            focus(-1, FocusGroup::box);
        else
            focus(clickedIndex + indexOffset, FocusGroup::box);
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
    bool target = t && t->getTarget();
    switch (state) {
    case UIState::traveling:
        if (target) {
            pause = true;
            setState(UIState::fighting);
        }
        break;
    case UIState::trading:
        traveler->updateTradeButtons(pagers);
        break;
    case UIState::fighting:
        if (!traveler->alive())
            setState(UIState::dying);
        else if (!target)
            setState(UIState::logging);
        else if (traveler->fightWon())
            setState(UIState::looting);
        else
            traveler->updateFightBoxes(pagers[0]);
        break;
    default:
        break;
    }
    int scrollX = 0, scrollY = 0;
    double sS = Settings::getScroll() * modMultiplier;
    if (show) {
        const SDL_Rect &mR = game.getMapRect();
        if (traveler->getPX() < int(mR.w * kShowPlayerPadding)) scrollX = -sS;
        if (traveler->getPY() < int(mR.h * kShowPlayerPadding)) scrollY = -sS;
        if (traveler->getPX() > int(mR.w * (1 - kShowPlayerPadding))) scrollX = sS;
        if (traveler->getPY() > int(mR.h * (1 - kShowPlayerPadding))) scrollY = sS;
    }
    std::for_each(begin(scroll), end(scroll), [&scrollX, &scrollY, sS](Direction dir) {
        switch (dir) {
        case left:
            scrollX -= sS;
            break;
        case right:
            scrollX += sS;
            break;
        case up:
            scrollY -= sS;
            break;
        case down:
            scrollY += sS;
            break;
        }
    });
    if (scrollX || scrollY) game.moveView(scrollX, scrollY);
    if (traveler.get() && !pause) traveler->update(e);
}

void Player::draw(SDL_Renderer *s) {
    if (traveler.get()) traveler->draw(s);
    for (auto &pgr : pagers) pgr.draw(s);
}
