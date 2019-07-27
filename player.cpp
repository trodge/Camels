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
    SDL_Rect sR = Settings::getScreenRect();
    SDL_Color uIFgr = Settings::getUiForeground(), uIBgr = Settings::getUiBackground(), uIHgl = Settings::getUiHighlight();
    int bBB = Settings::getBigBoxBorder(), bBR = Settings::getBigBoxRadius(), bBFS = Settings::getBigBoxFontSize(),
        sBB = Settings::getSmallBoxBorder(), sBR = Settings::getSmallBoxRadius(), sBFS = Settings::getSmallBoxFontSize();
    auto bigBox = [&uIFgr, &uIBgr, &uIHgl, bBB, bBR, bBFS](const SDL_Rect &rt, const std::vector<std::string> &tx) {
        return BoxInfo{rt, tx, uIFgr, uIBgr, uIHgl, 0u, false, bBB, bBR, bBFS, {}};
    };
    auto bigButton = [&uIFgr, &uIBgr, &uIHgl, bBB, bBR, bBFS](const SDL_Rect &rt, const std::vector<std::string> &tx,
                                                              SDL_Keycode ky, std::function<void(MenuButton *)> fn) {
        return BoxInfo{rt, tx, uIFgr, uIBgr, uIHgl, 0u, false, bBB, bBR, bBFS, {}, ky, fn};
    };
    auto bigSelectButton = [&uIFgr, &uIBgr, &uIHgl, bBB, bBR, bBFS](const SDL_Rect &rt, const std::vector<std::string> &tx,
                                                                    SDL_Keycode ky, std::function<void(MenuButton *)> fn) {
        return BoxInfo{rt, tx, uIFgr, uIBgr, uIHgl, 0u, false, bBB, bBR, bBFS, {}, ky, fn};
    };
    auto smallBox = [&uIFgr, &uIBgr, &uIHgl, sBB, sBR, sBFS](const SDL_Rect &rt, const std::vector<std::string> &tx) {
        return BoxInfo{rt, tx, uIFgr, uIBgr, uIHgl, 0u, false, sBB, sBR, sBFS, {}};
    };
    auto smallButton = [&uIFgr, &uIBgr, &uIHgl, sBB, sBR, sBFS](const SDL_Rect &rt, const std::vector<std::string> &tx,
                                                                SDL_Keycode ky, std::function<void(MenuButton *)> fn) {
        return BoxInfo{rt, tx, uIFgr, uIBgr, uIHgl, 0u, false, sBB, sBR, sBFS, {}, ky, fn};
    };
    uIStates = {
        {UIState::starting,
         {{bigBox({sR.w / 2, sR.h / 15, 0, 0}, {"Camels and Silk"}),
           bigButton({sR.w / 7, sR.h / 3, 0, 0}, {"(N)ew Game"}, SDLK_n,
                     [this, sR, bBB, bBR, bBFS](MenuButton *) {
                         game.newGame();
                         auto &nations = game.getNations();
                         auto &beginningBoxes = uIStates.at(UIState::beginning).boxesInfo;
                         for (auto &nt : nations)
                             // Create a button for each nation to start in that nation.
                             beginningBoxes.push_back({.rect = {sR.w * (static_cast<int>(nt.getId() - 1) % 3 * 2 + 1) / 6,
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
                                                           std::string name = pagers[0u].getVisible(0u)->getText(1u);
                                                           if (name.empty()) name = nt.randomName();
                                                           // Create traveler object for player
                                                           traveler = game.createPlayerTraveler(nId, name);
                                                           show = true;
                                                           focusBox = -1;
                                                           setState(UIState::traveling);
                                                       }});
                         setState(UIState::beginning);
                     }),
           bigButton({sR.w / 7, sR.h * 2 / 3, 0, 0}, {"(L)oad Game"}, SDLK_l,
                     [this](MenuButton *) { setState(UIState::loading); })}}},
        {UIState::beginning,
         {{bigBox({sR.w / 2, sR.h / 7, 0, 0}, {"Name", ""})},
          [this] {
              // Allow typing in name box.
              pagers[0u].toggleLock(0u);
          }}},
        {UIState::quitting,
         {{bigButton({sR.w / 2, sR.h / 4, 0, 0}, {"Continue"}, SDLK_ESCAPE,
                     [this](MenuButton *) {
                         pause = storedPause;
                         setState(storedState);
                     }),
           traveler ? bigButton({sR.w / 2, sR.h * 3 / 4, 0, 0}, {"Save and Quit"}, SDLK_q,
                                [this](MenuButton *) {
                                    game.saveGame();
                                    stop = true;
                                }) :
                      bigButton({sR.w / 2, sR.h * 3 / 4, 0, 0}, {"Quit"}, SDLK_q, [this](MenuButton *) { stop = true; })},
          [this] {
              storedState = state;
              storedPause = pause;
          }}},
        {UIState::loading,
         {{bigButton({sR.w / 7, sR.h / 7, 0, 0}, {"(B)ack"}, SDLK_b,
                     [this](MenuButton *) { setState(UIState::starting); })}}},
        {UIState::traveling,
         {{smallButton({sR.w / 9, sR.h * 14 / 15, 0, 0}, {"(G)o"}, SDLK_g,
                       [this](MenuButton *btn) {
                           if (focusTown > -1) game.pickTown(traveler, static_cast<size_t>(focusTown));
                           show = true;
                           btn->setClicked(false);
                       }),
           smallButton({sR.w * 2 / 9, sR.h * 14 / 15, 0, 0}, {"(T)rade"}, SDLK_t,
                       [this](MenuButton *) { setState(UIState::trading); }),
           smallButton({sR.w / 3, sR.h * 14 / 15, 0, 0}, {"(S)tore"}, SDLK_s,
                       [this](MenuButton *) { setState(UIState::storing); }),
           smallButton({sR.w * 4 / 9, sR.h * 14 / 15, 0, 0}, {"(M)anage"}, SDLK_m,
                       [this](MenuButton *) { setState(UIState::managing); }),
           smallButton({sR.w * 5 / 9, sR.h * 14 / 15, 0, 0}, {"(E)quip"}, SDLK_e,
                       [this](MenuButton *) { setState(UIState::equipping); }),
           smallButton({sR.w * 2 / 3, sR.h * 14 / 15, 0, 0}, {"(H)ire"}, SDLK_h,
                       [this](MenuButton *) { setState(UIState::hiring); }),
           smallButton({sR.w * 7 / 9, sR.h * 14 / 15, 0, 0}, {"(A)ttack"}, SDLK_a,
                       [this](MenuButton *) { setState(UIState::attacking); }),
           smallButton({sR.w * 8 / 9, sR.h * 14 / 15, 0, 0}, {"(L)og"}, SDLK_l,
                       [this](MenuButton *) { setState(UIState::logging); })},
          [this] { pause = false; }}},
        {UIState::trading,
         {{smallButton({sR.w * 2 / 9, sR.h * 14 / 15, 0, 0}, {"Stop (T)rading"}, SDLK_t,
                       [this](MenuButton *) { setState(UIState::traveling); }),
           smallBox({sR.w * 4 / 9, sR.h * 13 / 15, 0, 0}, {""}),
           smallButton({sR.w * 4 / 9, sR.h * 14 / 15, 0, 0}, {"(S)et Portion"}, SDLK_s,
                       [this](MenuButton *btn) {
                           double p;
                           TextBox *portionBox = pagers[0u].getVisible(kPortionBoxIndex);
                           std::stringstream{portionBox->getText(0u)} >> p;
                           traveler->setPortion(p);
                           traveler->updatePortionBox(portionBox);
                           btn->setClicked(false);
                       }),
           smallButton({sR.w * 2 / 9, sR.h * 13 / 15, 0, 0}, {"(C)omplete Trade"}, SDLK_c,
                       [this](MenuButton *) {
                           traveler->makeTrade();
                           setState(UIState::trading);
                       })},
          [this] {
              traveler->updatePortionBox(pagers[0u].getVisible(kPortionBoxIndex));
              traveler->createTradeButtons(pagers, printer);
          },
          3u}},
        {UIState::storing,
         {{smallButton({sR.w / 3, sR.h * 14 / 15, 0, 0}, {"Stop (S)toring"}, SDLK_s,
                       [this](MenuButton *) { setState(UIState::traveling); }),
           smallBox({sR.w * 4 / 9, sR.h * 13 / 15, 0, 0}, {""}),
           smallButton({sR.w * 4 / 9, sR.h * 14 / 15, 0, 0}, {"(S)et Portion"}, SDLK_s,
                       [this](MenuButton *btn) {
                           double p;
                           TextBox *portionBox = pagers[0u].getVisible(kPortionBoxIndex);
                           std::stringstream{portionBox->getText(0u)} >> p;
                           traveler->setPortion(p);
                           traveler->updatePortionBox(portionBox);
                           btn->setClicked(false);
                       })},
          [this] {
              traveler->updatePortionBox(pagers[0u].getVisible(kPortionBoxIndex));
              traveler->createStorageButtons(pagers, focusBox, printer);
          },
          3u}},
        {UIState::managing,
         {{smallButton({sR.w * 4 / 9, sR.h * 14 / 15, 0, 0}, {"Stop (M)anaging"}, SDLK_m,
                       [this](MenuButton *) { setState(UIState::traveling); })},
          [this] { traveler->createManageButtons(pagers, printer); },
          3u}},
        {UIState::equipping,
         {{smallButton({sR.w * 5 / 9, sR.h * 14 / 15, 0, 0}, {"Stop (E)quipping"}, SDLK_e,
                       [this](MenuButton *) { setState(UIState::traveling); })},
          [this] { traveler->createEquipButtons(pagers, focusBox, printer); }}},
        {UIState::hiring,
         {{smallButton({sR.w * 2 / 3, sR.h * 14 / 15, 0, 0}, {"Stop (H)iring"}, SDLK_h,
                       [this](MenuButton *) { setState(UIState::traveling); })}}},
        {UIState::attacking,
         {{smallButton({sR.w * 7 / 9, sR.h * 14 / 15, 0, 0}, {"Cancel (A)ttack"}, SDLK_a,
                       [this](MenuButton *) { setState(UIState::traveling); })},
          [this] {
              pause = true;
              traveler->createAttackButton(
                  pagers[0u], [this] { setState(UIState::fighting); }, printer);
          }}},
        {UIState::logging,
         {{smallButton({sR.w * 8 / 9, sR.h * 14 / 15, 0, 0}, {"Close (L)og"}, SDLK_l,
                       [this](MenuButton *) { setState(UIState::traveling); })},
          [this, sR, bBB, bBR, sBFS] {
              // Create log box.
              pagers[0].addBox(std::make_unique<ScrollBox>(BoxInfo{{sR.w / 15, sR.h * 2 / 15, sR.w * 13 / 15, sR.h * 11 / 15},
                                                                   traveler->getLogText(),
                                                                   traveler->getNation()->getForeground(),
                                                                   traveler->getNation()->getBackground(),
                                                                   traveler->getNation()->getHighlight(),
                                                                   0u,
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
        {UIState::fighting, {{}, [this] { traveler->createFightBoxes(pagers[0u], pause, printer); }}},
        {UIState::looting,
         {{smallButton({sR.w / 15, sR.h * 14 / 15, 0, 0}, {"(D)one Looting"}, SDLK_d,
                       [this](MenuButton *) {
                           traveler->loseTarget();
                           setState(UIState::traveling);
                       }),
           smallButton({sR.w * 4 / 15, sR.h * 14 / 15, 0, 0}, {"(L)oot All"}, SDLK_l,
                       [this](MenuButton *) {
                           auto tgt = traveler->getTarget().lock();
                           traveler->loot(*tgt);
                           traveler->loseTarget();
                           setState(UIState::traveling);
                       })},
          [this] { traveler->createLootButtons(pagers, focusBox, printer); },
          3u}},
        {UIState::dying, {{}, [this, &sR, bBB, bBR, bBFS] {
                              pagers[0u].addBox(std::make_unique<TextBox>(
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
        .boxesInfo.push_back(bigSelectButton({sR.w / 5, sR.h / 7, sR.w * 3 / 5, sR.h * 5 / 7}, saves, SDLK_l,
                                             [this, &path](MenuButton *btn) {
                                                 game.loadGame((path / btn->getItem()).replace_extension("sav"));
                                                 show = true;
                                                 setState(UIState::traveling);
                                             }));
}

void Player::loadTraveler(const Save::Traveler *t, std::vector<Town> &ts) {
    // Load the traveler for the player from save file.
    traveler = std::make_shared<Traveler>(t, ts, game.getNations(), game.getData());
}

void Player::prepFocus(FocusGroup g, int &i, int &s, std::vector<TextBox *> &fcbls) {
    std::vector<Town *> neighbors;
    switch (g) {
    case box:
        i = focusBox;
        for (auto &pg : pagers) {
            // Put all visible boxes from all pages in focusables vector.
            auto bxs = pg.getVisible();
            fcbls.insert(fcbls.end(), bxs.begin(), bxs.end());
        }
        s = static_cast<int>(fcbls.size());
        break;
    case neighbor:
        neighbors = traveler->getTown()->getNeighbors();
        i = static_cast<int>(
            std::find_if(neighbors.begin(), neighbors.end(),
                         [this](Town *t) { return t->getId() == static_cast<unsigned int>(focusTown + 1); }) -
            neighbors.begin());
        s = static_cast<int>(neighbors.size());
        if (i == s) {
            // Currently focused town is last neighbor.
            auto &ts = game.getTowns();
            size_t fT = focusTown;
            if (fT < ts.size()) ts[fT].getBox()->changeBorder(-kTownDB);
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

void Player::recedePage() {

}

void Player::advancePage() {

}

void Player::setState(UIState::State s) {
    // Change the UI state to s.
    UIState newState = uIStates.at(s);
    pagers.clear();
    pagers.resize(newState.pagerCount);
    currentPager = pagers.begin();
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
    if (mod & (KMOD_CTRL | KMOD_SHIFT | KMOD_ALT))
        modMultiplier = 10000.;
    else if (mod & (KMOD_CTRL | KMOD_ALT))
        modMultiplier = 0.001;
    else if (mod & (KMOD_SHIFT | KMOD_ALT))
        modMultiplier = 0.01;
    else if (mod & (KMOD_CTRL | KMOD_SHIFT))
        modMultiplier = 1000.;
    else if (mod & KMOD_ALT)
        modMultiplier = 0.1;
    else if (mod & KMOD_SHIFT)
        modMultiplier = 10.;
    else if (mod & KMOD_CTRL)
        modMultiplier = 100.;
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
        // Event is SDL_KEYDOWN
        if (keyedIndex < 0) {
            // No box captured the key press.
            if (state != UIState::quitting) {
                if (traveler) {
                    int columnCount = state == UIState::managing ?
                        kBusinessButtonXDivisor / kBusinessButtonSpaceMultiplier / 2 :
                        kGoodButtonXDivisor / kGoodButtonSpaceMultiplier / 2;
                    switch (state) {
                    case UIState::traveling:
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
                    case UIState::trading:
                    case UIState::storing:
                    case UIState::managing:
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
                            traveler->updatePortionBox(pagers[0u].getVisible(kPortionBoxIndex));
                            break;
                        case SDLK_PERIOD:
                            traveler->changePortion(0.1);
                            traveler->updatePortionBox(pagers[0u].getVisible(kPortionBoxIndex));
                            break;
                        case SDLK_LEFTBRACKET:
                            recedePage();
                            break;
                        case SDLK_RIGHTBRACKET:
                            advancePage();
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
            case SDLK_TAB:
                if (mod & KMOD_SHIFT)
                    focusPrev(box);
                else
                    focusNext(box);
                break;
            }
        } else
            // A box was keyed down.
            focus(keyedIndex + indexOffset, FocusGroup::box);
    } else {
        // Event is SDL_KEYUP.
        if (keyedIndex < 0)
            // Key is not used by any box.
            switch (k.keysym.sym) {
            case SDLK_LEFT:
            case SDLK_RIGHT:
            case SDLK_UP:
            case SDLK_DOWN:
                scrollKeys.erase(k.keysym.sym);
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
        auto clickedTown =
            std::find_if(fcbls.begin(), fcbls.end(), [&b](const TextBox *t) { return t->clickCaptured(b); });
        if (clickedTown != fcbls.end()) focus(static_cast<int>(std::distance(fcbls.begin(), clickedTown)), town);
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
    bool target = t && t->getTarget().lock();
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
        if (!traveler->alive()) {
            setState(UIState::dying);
            break;
        } else if (!target) {
            setState(UIState::logging);
            break;
        } else if (traveler->fightWon()) {
            setState(UIState::looting);
            break;
        }
        traveler->updateFightBoxes(pagers[0u]);
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

void Player::draw(SDL_Renderer *s) {
    if (traveler.get()) traveler->draw(s);
    for (auto &pgr : pagers) pgr.draw(s);
}
