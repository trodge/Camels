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
#include <numeric>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

#include "game.hpp"
#include "menubutton.hpp"
#include "pager.hpp"
#include "scrollbox.hpp"
#include "selectbutton.hpp"
#include "settings.hpp"
#include "textbox.hpp"
#include "traveler.hpp"

class Game;

class MenuButton;

struct UIState {
    enum State {
        starting,
        beginning,
        quitting,
        loading,
        traveling,
        trading,
        storing,
        building,
        equipping,
        managing,
        attacking,
        logging,
        fighting,
        looting,
        dying
    };
    std::vector<BoxInfo> boxesInfo;           // info for boxes to create for this state
    std::function<void()> onChange = nullptr; // function to run when this UIState is switched to
    size_t pagerCount = 1;                    // number of pagers this state uses
};

class Player {
    std::unique_ptr<Traveler> traveler;
    std::vector<Pager> pagers;
    std::vector<Pager>::iterator currentPager;
    Game &game;
    Printer &printer;
    bool stop = false, show = false, pause = false, storedPause = false, developer = false;
    enum Direction { left, right, up, down };
    std::unordered_set<Direction> scroll;
    double modMultiplier = 1.; // multiplier for values which depend on keymod state
    int focusBox = -1;         // index of box we are focusing across all pagers
    int focusTown = -1;        // index of town currently focused
    UIState::State state = UIState::starting, storedState = UIState::starting;
    std::unordered_map<UIState::State, UIState> uIStates;
    enum FocusGroup { box, neighbor, town };
    void prepFocus(FocusGroup g, int &i, int &s, std::vector<TextBox *> &fcbls);
    void finishFocus(int f, FocusGroup g, const std::vector<TextBox *> &fcbls);
    void focus(int f, FocusGroup g);
    void focusPrev(FocusGroup g);
    void focusNext(FocusGroup g);
    void createBoxes(UIState::State s);
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
    void loadTraveler(const Save::Traveler *t, std::vector<Town> &ts);
    void setState(UIState::State s);
    void place(int ox, int oy, double s) {
        if (traveler.get()) traveler->place(ox, oy, s);
    }
    void handleEvent(const SDL_Event &e);
    void update(unsigned int e);
    void draw(SDL_Renderer *s);
};

#endif
