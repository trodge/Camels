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
#include <functional>
#include <memory>
#include <unordered_set>
#include <vector>

#include "game.hpp"
#include "selectbutton.hpp"
#include "traveler.hpp"

class Game;

class Player {
    std::shared_ptr<Traveler> traveler;
    std::vector<std::unique_ptr<TextBox>> boxes, storedBoxes;
    int focusBox = -1; // index of box currently focused, -1 if no focus
    Game &game;
    Printer &printer;
    bool stop = false, show = false, pause = false, storedPause = true;
    std::unordered_set<SDL_Keycode> scrollKeys;
    double modMultiplier;
    int focusTown = -1;                          // index of town currently focused, -1 if no focus
    size_t offerButtonIndex, requestButtonIndex; // index of first offer and request button for updating trade buttons
    size_t portionBoxIndex;                      // index of box for setting portion
    size_t setPortionButtonIndex;                // index of button for setting portion
    size_t storageButtonIndex;                   // index of first storage button
    size_t equipButtonIndex;                     // index of first equip button
    size_t lootButtonIndex;                      // index of first loot button
    enum UIState {
        starting,
        beginning,
        quitting,
        loading,
        traveling,
        trading,
        storing,
        managing,
        equipping,
        hiring,
        attacking,
        fighting,
        looting,
        logging,
        dying
    };
    UIState state = starting, storedState = starting;
    struct StateTextKey {
        UIState state;
        std::string text;
        SDL_Keycode key;
    };
    std::array<StateTextKey, 7> stateTextKeys;
    void prepFocus(Focusable::FocusGroup g, int &i, int &s, std::vector<Focusable *> &fcbls);
    void finishFocus(int f, Focusable::FocusGroup g, const std::vector<Focusable *> &fcbls);
    void focus(int f, Focusable::FocusGroup g);
    void focusPrev(Focusable::FocusGroup g);
    void focusNext(Focusable::FocusGroup g);
    void updatePortionBox();
    void createStorageView(const Town *t);
    void setState(UIState s);
    void toggleState(UIState s);
    void handleKey(const SDL_KeyboardEvent &k);
    void handleTextInput(const SDL_TextInputEvent &t);
    void handleClick(const SDL_MouseButtonEvent &b);

  public:
    Player(Game &g);
    bool getStop() const { return stop; }
    bool getPause() const { return pause; }
    bool getShow() const { return show; }
    const Traveler *getTraveler() const { return traveler.get(); }
    bool hasTraveler() const { return traveler.get(); }
    void start() { setState(starting); }
    void loadTraveler(const Save::Traveler *t, std::vector<Town> &ts);
    void place(int ox, int oy, double s) {
        if (traveler.get())
            traveler->place(ox, oy, s);
    }
    void handleEvent(const SDL_Event &e);
    void update(unsigned int e);
    void draw(SDL_Renderer *s);
};

#endif
