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

int main(int, char **) {
    // Start SDL, SDL_ttf, and SDL_image.
    if (SDL_Init(SDL_INIT_EVERYTHING) < 0)
        std::cout << "SDL initialization failed, SDL Error: " << SDL_GetError() << std::endl;
    if (TTF_Init() < 0)
        std::cout << "TTF initialization failed, TTF Error: " << TTF_GetError() << std::endl;
    if (IMG_Init(IMG_INIT_PNG) < 0)
        std::cout << "TTF initialization failed, IMG Error: " << SDL_GetError() << std::endl;
    Settings::load("settings.ini");
    {
        Game game;
        game.run();
    }
    Settings::save("settings.ini");
    std::cout << "Quitting TTF" << std::endl;
    TTF_Quit();
    std::cout << "Quitting SDL" << std::endl;
    SDL_Quit();
    return 0;
}
