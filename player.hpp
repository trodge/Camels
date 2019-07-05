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

#ifndef PLAYER_H
#define PLAYER_H
#include <vector>
#include <memory>
#include <functional>

#include "traveler.hpp"
#include "selectbutton.hpp"


class Player {
    std::shared_ptr<Traveler> traveler;
    std::vector<std::unique_ptr<TextBox>> boxes, storedBoxes;
    int focusBox = -1; // index of box currently focused, -1 if no focus
    // references to game object members
    bool &stop;
    bool &pause;
    bool &storedPause;
    std::vector<Town> &towns;
    const std::vector<Nation> &nations;
    int &scrollX, &scrollY;
    bool &showPlayer;
    const GameData &gameData;
    std::function<void()> newGame;
    std::function<void(const fs::path &)> loadGame;
    std::function<void()> saveGame;
    std::function<void()> saveData;
    std::function<void()> saveRoutes;
    std::function<void()> place;
    int focusTown = -1; // index of town currently focused, -1 if no focus
    size_t offerButtonIndex, requestButtonIndex; // index of first offer and request button for updating trade buttons
    size_t portionBoxIndex;            // index of box for setting portion
    size_t setPortionButtonIndex; // index of button for setting portion
    size_t equipButtonIndex; // index of first equip button
    enum UIState {
        starting,
        beginning,
        quitting,
        loading,
        traveling,
        logging,
        trading,
        storing,
        managing,
        equipping,
        attacking,
        fighting,
        looting,
        dying
    };
    UIState state = starting, storedState = starting;
    void prepFocus(Focusable::FocusGroup g, int &i, int &s, std::vector<Focusable *> &fcbls);
    void finishFocus(int f, Focusable::FocusGroup g, const std::vector<Focusable *> &fcbls);
    void focus(int f, Focusable::FocusGroup g);
    void focusPrev(Focusable::FocusGroup g);
    void focusNext(Focusable::FocusGroup g);
    void createStartButtons();
    void createBeginButtons();
    void createQuitButtons();
    void createLoadButtons();
    void createTravelButtons();
    void createLogBox();
    void createTradeButtons();
    void updateTradeButtons();
    void updatePortionBox();
    void createStorageView(const Town *t);
    void createEquipButtons();
    void createStorageButtons();
    void createManageButtons();
    void createAttackButton();
    void createFightBoxes();
    void updateFightBoxes();
    void createLootButtons();
    void createDyingBoxes();
    void setState(UIState s);
    void toggleState(UIState s);
 public:
    Player(bool &s, bool &p, bool &sP, std::vector<Town> &ts, std::vector<Nation> &ns, int &sX, int &sY, bool &shP, GameData &gD);
    void setFunctions(std::function<void()> nG, std::function<void(const fs::path &)> lG, std::function<void()> sG, std::function<void()> sD, std::function<void()> sR, std::function<void()> p);
    void loadTraveler(const Save::Traveler *t);
    int getPX() { return traveler->getPX(); }
    int getPY() { return traveler->getPY(); }
    void handleKey(const SDL_KeyboardEvent &k, SDL_Keymod mod);
    void handleTextInput(const SDL_TextInputEvent &t);
    void handleClick(const SDL_MouseButtonEvent &b);
    void updateUI();
    void draw(SDL_Renderer *s);
};

#endif