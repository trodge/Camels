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

Player::Player(Game &g) : game(g), screenRect(Settings::getScreenRect()), printer(g.getPrinter()) {
    printer.setSize(Settings::boxSize(BoxSize::small));
    smallBoxFontHeight = printer.getFontHeight();
    int bBB = Settings::boxSize(BoxSize::big).border;
    uIStates = {
        {UIState::starting,
         {{Settings::boxInfo({screenRect.w / 2, screenRect.h / 15, 0, 0}, {"Camels and Silk"}, BoxSize::big),
           Settings::boxInfo({screenRect.w / 7, screenRect.h / 3, 0, 0}, {"(N)ew Game"}, BoxSize::big, SDLK_n,
                                [this](MenuButton *) {
                                    game.newGame();
                                    auto &nations = game.getNations();
                                    auto &beginningBoxes = uIStates.at(UIState::beginning).boxesInfo;
                                    for (auto &nt : nations)
                                        // Create a button for each nation to start in that nation.
                                        beginningBoxes.push_back(Settings::boxInfo({
                                                      screenRect.w * (static_cast<int>(nt.getId() - 1) % 3 * 2 + 1) / 6,
                                                      screenRect.h * (static_cast<int>(nt.getId() - 1) / 3 + 2) / 7, 0, 0},
                                                      nt.getNames(), nt.getColors(), {nt.getId(), true}, BoxSize::big, SDLK_UNKNOWN,
                                                      [this, nt](MenuButton *) {
                                                        unsigned int nId = nt.getId(); // nation id
                                                        std::string name = pagers[0].getVisible(0)->getText(1);
                                                        if (name.empty()) name = nt.randomName();
                                                        // Create traveler object for player
                                                        traveler = game.createPlayerTraveler(nId, name);
                                                        traveler->properties.find(0)->second.create(55, 0.75);
                                                        traveler->properties.find(0)->second.create(59, 2);
                                                        show = true;
                                                        focusBox = -1;
                                                        setState(UIState::traveling);
                                                      }));
                                    setState(UIState::beginning);
                                }),
           Settings::boxInfo({screenRect.w / 7, screenRect.h * 2 / 3, 0, 0}, {"(L)oad Game"}, BoxSize::big, SDLK_l,
                                [this](MenuButton *) { setState(UIState::loading); })}}},
        {UIState::beginning, {{Settings::boxInfo({screenRect.w / 2, screenRect.h / 7, 0, 0}, {"Name", ""}, BoxSize::big, BoxInfo::edit)}}},
        {UIState::quitting,
         {{Settings::boxInfo({screenRect.w / 2, screenRect.h * 3 / 4, 0, 0}, {""}, BoxSize::big, SDLK_q,
                                [this](MenuButton *) {
                                    if (traveler) game.saveGame();
                                    stop = true;
                                }),
           Settings::boxInfo({screenRect.w / 2, screenRect.h / 4, 0, 0}, {"Continue"}, BoxSize::big, SDLK_ESCAPE,
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
         {{Settings::boxInfo({screenRect.w / 7, screenRect.h / 7, 0, 0}, {"(B)ack"}, BoxSize::big, SDLK_b,
                                [this](MenuButton *) { setState(UIState::starting); })}}},
        {UIState::traveling,
         {{Settings::boxInfo({screenRect.w / 9, screenRect.h - smallBoxFontHeight, 0, 0}, {"(G)o"}, BoxSize::small, SDLK_g,
                                [this](MenuButton *btn) {
                                    if (focusTown > -1) game.pickTown(traveler.get(), static_cast<size_t>(focusTown));
                                    show = true;
                                    btn->setClicked(false);
                                }),
           Settings::boxInfo({screenRect.w * 2 / 9, screenRect.h - smallBoxFontHeight, 0, 0}, {"(T)rade"}, BoxSize::small, SDLK_t,
                                [this](MenuButton *) { setState(UIState::trading); }),
           Settings::boxInfo({screenRect.w / 3, screenRect.h - smallBoxFontHeight, 0, 0}, {"(S)tore"}, BoxSize::small, SDLK_s,
                                [this](MenuButton *) { setState(UIState::storing); }),
           Settings::boxInfo({screenRect.w * 4 / 9, screenRect.h - smallBoxFontHeight, 0, 0}, {"(B)uild"}, BoxSize::small, SDLK_b,
                                [this](MenuButton *) { setState(UIState::building); }),
           Settings::boxInfo({screenRect.w * 5 / 9, screenRect.h - smallBoxFontHeight, 0, 0}, {"(E)quip"}, BoxSize::small, SDLK_e,
                                [this](MenuButton *) { setState(UIState::equipping); }),
           Settings::boxInfo({screenRect.w * 2 / 3, screenRect.h - smallBoxFontHeight, 0, 0}, {"(M)anage"}, BoxSize::small, SDLK_m,
                                [this](MenuButton *) { setState(UIState::managing); }),
           Settings::boxInfo({screenRect.w * 7 / 9, screenRect.h - smallBoxFontHeight, 0, 0}, {"(A)ttack"}, BoxSize::small, SDLK_a,
                                [this](MenuButton *) { setState(UIState::attacking); }),
           Settings::boxInfo({screenRect.w * 8 / 9, screenRect.h - smallBoxFontHeight, 0, 0}, {"(L)og"}, BoxSize::small, SDLK_l,
                                [this](MenuButton *) { setState(UIState::logging); })},
          [this] { pause = false; }}},
        {UIState::trading,
         {{Settings::boxInfo({screenRect.w * 2 / 9, screenRect.h - smallBoxFontHeight, 0, 0}, {"Stop (T)rading"}, BoxSize::small, SDLK_t,
                                [this](MenuButton *) { setState(UIState::traveling); }),
           Settings::boxInfo({screenRect.w * 2 / 9, screenRect.h - (smallBoxFontHeight + bBB) * 2, 0, 0}, {"(C)omplete Trade"}, 
                                BoxSize::small, SDLK_c,
                                [this](MenuButton *) {
                                    traveler->makeTrade();
                                    setState(UIState::trading);
                                })

          },
          [this] {
              traveler->createTradeButtons(pagers, printer);
          },
          3}},
        {UIState::storing,
         {{Settings::boxInfo({screenRect.w / 3, screenRect.h - smallBoxFontHeight, 0, 0}, {"Stop (S)toring"}, BoxSize::small, SDLK_s,
                                [this](MenuButton *) { setState(UIState::traveling); })},
          [this] {
              traveler->createStorageButtons(pagers, focusBox, printer);
          },
          3}},
        {UIState::building,
         {{Settings::boxInfo({screenRect.w * 4 / 9, screenRect.h - smallBoxFontHeight, 0, 0}, {"Stop (B)uilding"}, BoxSize::small, SDLK_b,
                                [this](MenuButton *) { setState(UIState::traveling); })},
          [this] {
              traveler->createBuildButtons(pagers, focusBox, printer);
          },
          3}},
        {UIState::equipping,
         {{Settings::boxInfo({screenRect.w * 5 / 9, screenRect.h - smallBoxFontHeight, 0, 0}, {"Stop (E)quipping"}, BoxSize::small, SDLK_e,
                                [this](MenuButton *) { setState(UIState::traveling); })},
          [this] { traveler->createEquipButtons(pagers, focusBox, printer); },
          2}},
        {UIState::managing,
         {{Settings::boxInfo({screenRect.w * 2 / 3, screenRect.h - smallBoxFontHeight, 0, 0}, {"Stop (M)anaging"}, BoxSize::small, SDLK_m,
                                [this](MenuButton *) { setState(UIState::traveling); })}}},
        {UIState::attacking,
         {{Settings::boxInfo({screenRect.w * 7 / 9, screenRect.h - smallBoxFontHeight, 0, 0}, {"Cancel (A)ttack"}, BoxSize::small, SDLK_a,
                                [this](MenuButton *) { setState(UIState::traveling); })},
          [this] {
              pause = true;
              traveler->createAttackButton(
                  pagers[0], [this] { setState(UIState::fighting); }, printer);
          }}},
        {UIState::logging,
         {{Settings::boxInfo({screenRect.w * 8 / 9, screenRect.h - smallBoxFontHeight, 0, 0}, {"Close (L)og"}, BoxSize::small, SDLK_l,
                                [this](MenuButton *) { setState(UIState::traveling); })},
          [this] {
              traveler->createLogBox(pagers[0], printer);
          }}},
        {UIState::fighting, {{}, [this] { traveler->createFightBoxes(pagers[0], pause, printer); }}},
        {UIState::looting,
         {{Settings::boxInfo({screenRect.w / 15, screenRect.h - smallBoxFontHeight, 0, 0}, {"Stop (L)ooting"}, BoxSize::small, SDLK_l,
                                [this](MenuButton *) {
                                    traveler->loseTarget();
                                    setState(UIState::traveling);
                                }),
           Settings::boxInfo({screenRect.w * 4 / 15, screenRect.h - smallBoxFontHeight, 0, 0}, {"Loot (A)ll"}, BoxSize::small, SDLK_a,
                                [this](MenuButton *) {
                                    traveler->loot();
                                    traveler->loseTarget();
                                    setState(UIState::traveling);
                                })},
          [this] {
              traveler->createLootButtons(pagers, focusBox, printer);
          },
          3}},
        {UIState::dying, {{}, [this] {
                              pagers[0].addBox(std::make_unique<TextBox>(
                                  Settings::boxInfo({screenRect.w / 2, screenRect.h / 2, 0, 0},
                                          {traveler->getLogText().back(), "You have died."},
                                          traveler->getNation()->getColors(),
                                          BoxSize::big),
                                  printer));
                          }}}};

    fs::path path{"save"};
    std::vector<std::string> saves;
    for (auto &file : fs::directory_iterator{path}) saves.push_back(file.path().stem().string());
    uIStates.at(UIState::loading)
        .boxesInfo.push_back(Settings::boxInfo({screenRect.w / 5, screenRect.h / 7, screenRect.w * 3 / 5, screenRect.h * 5 / 7},
                                               saves, BoxSize::big, BoxInfo::scroll, SDLK_l, [this, &path](MenuButton *btn) {
                                                   game.loadGame((path / btn->getItem()).replace_extension("sav"));
                                                   show = true;
                                                   setState(UIState::traveling);
                                               }));
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
            if (fcbls[static_cast<size_t>(i)]->canFocus()) break;
        if (i < 0)
            // No focusable item found in group.
            return;
    } else {
        fcbls[static_cast<size_t>(i)]->toggleFocus();
        int j = i;
        while (--i != j)
            if (i < 0)
                i = s;
            else if (fcbls[static_cast<size_t>(i)]->canFocus())
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
            if (fcbls[static_cast<size_t>(i)]->canFocus()) break;
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
            else if (fcbls[static_cast<size_t>(i)]->canFocus())
                break;
    }
    fcbls[static_cast<size_t>(i)]->toggleFocus();
    finishFocus(i, g, fcbls);
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
        if (bI.onClick && bI.behavior == BoxInfo::scroll)
            pagers[0].addBox(std::make_unique<SelectButton>(bI, printer));
        else if (bI.onClick)
            pagers[0].addBox(std::make_unique<MenuButton>(bI, printer));
        else if (bI.behavior == BoxInfo::scroll)
            pagers[0].addBox(std::make_unique<ScrollBox>(bI, printer));
        else
            pagers[0].addBox(std::make_unique<TextBox>(bI, printer));
    if (newState.onChange) newState.onChange();
    // Create boxes shared by multiple states.
    if (newState.pagerCount > 2) {
        int bBB = Settings::boxSize(BoxSize::big).border;
        // Create portion box and set portion button.
        pagers[0].addBox(std::make_unique<TextBox>(
            Settings::boxInfo({screenRect.w / 9, screenRect.h - smallBoxFontHeight, 0, 0}, {""}, BoxSize::small, BoxInfo::edit), printer));
        portionBox = pagers[0].getVisible().back();
        traveler->updatePortionBox(portionBox);
        pagers[0].addBox(std::make_unique<MenuButton>(
            Settings::boxInfo({screenRect.w / 9, screenRect.h - (smallBoxFontHeight + bBB) * 2, 0, 0},
                              {"(S)et Portion"}, BoxSize::small, SDLK_s,
                              [this](MenuButton *btn) {
                                  double p;
                                  std::stringstream{portionBox->getText(0)} >> p;
                                  traveler->setPortion(p);
                                  traveler->updatePortionBox(portionBox);
                                  btn->setClicked(false);
                              }),
            printer));
        // Create page switching buttons.
        std::array<SDL_Keycode, 4> keys{SDLK_MINUS, SDLK_EQUALS, SDLK_LEFTBRACKET, SDLK_RIGHTBRACKET};
        auto kyIt = keys.begin();
        for (auto pgIt = begin(pagers) + 1; pgIt != end(pagers); ++pgIt) {
            auto &bnds = pgIt->getBounds();
            // Create label.
            auto typeText = s == UIState::building ? "Businesses" : "Goods";
            pagers[0].addBox(std::make_unique<TextBox>(
                Settings::boxInfo({bnds.x + bnds.w / 2, bnds.y - smallBoxFontHeight, 0, 0},
                                  {pgIt == begin(pagers) + 1 ?
                                       traveler->getName() + "'s " + typeText :
                                       s == UIState::storing ?
                                       "Goods in " + traveler->toTown->getName() :
                                       (s == UIState::looting ? traveler->target->getName() : traveler->toTown->getName()) + "'s " + typeText},
                                  BoxSize::small),
                printer));
            if (pgIt->pageCount() > 1) {
                // Create page buttons.
                pagers[0].addBox(std::make_unique<MenuButton>(
                    Settings::boxInfo({bnds.x + bnds.w / 6, bnds.y + bnds.h + smallBoxFontHeight, 0, 0},
                                      {"Previous Page"}, BoxSize::small, *kyIt++,
                                      [pgIt](MenuButton *btn) {
                                          pgIt->recedePage();
                                          btn->setClicked(false);
                                      }),
                    printer));
                pagers[0].addBox(std::make_unique<MenuButton>(
                    Settings::boxInfo({bnds.x + bnds.w * 5 / 6, bnds.y + bnds.h + smallBoxFontHeight, 0, 0},
                                      {"Next Page"}, BoxSize::small, *kyIt++,
                                      [pgIt](MenuButton *btn) {
                                          pgIt->advancePage();
                                          btn->setClicked(false);
                                      }),
                    printer));
            } else
                kyIt += 2;
        }
    }
    focusNext(FocusGroup::box);
    state = s;
}

void Player::handleKey(const SDL_KeyboardEvent &k) {
    SDL_Keymod mod = SDL_GetModState();
    if ((mod & KMOD_CTRL) && (mod & KMOD_ALT) && (mod & KMOD_SHIFT))
        modMultiplier = 10000;
    else if ((mod & KMOD_CTRL) && (mod & KMOD_ALT))
        modMultiplier = 0.001;
    else if ((mod & KMOD_CTRL) && (mod & KMOD_SHIFT))
        modMultiplier = 1000;
    else if ((mod & KMOD_ALT) && (mod & KMOD_SHIFT))
        modMultiplier = 0.01;
    else if (mod & KMOD_CTRL)
        modMultiplier = 100;
    else if (mod & KMOD_ALT)
        modMultiplier = 0.1;
    else if (mod & KMOD_SHIFT)
        modMultiplier = 10;
    else
        modMultiplier = 1;
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
                int columnCount =
                    state == UIState::building ? Settings::getBusinessButtonColumns() : Settings::getGoodButtonColumns();
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
                        case SDLK_z:
                            traveler->resetTown();
                            break;
                        case SDLK_x:
                            traveler->toggleMaxGoods();
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
                        traveler->updatePortionBox(portionBox);
                        break;
                    case SDLK_PERIOD:
                        traveler->changePortion(0.1);
                        traveler->updatePortionBox(portionBox);
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
    if (b.state == SDL_PRESSED) {
        if (clickedIndex < 0)
            focus(-1, FocusGroup::box);
        else
            focus(clickedIndex + indexOffset, FocusGroup::box);
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
        const SDL_Rect &mR = game.getMapView();
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
