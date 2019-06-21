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

#ifndef FOCUSABLE_H
#define FOCUSABLE_H

#include "printer.h"

class Focusable {
  protected:
    bool canFocus = false;

  public:
    enum FocusGroup { box, neighbor, town };
    bool getCanFocus() const { return canFocus; }
    void toggleLock() { canFocus = not canFocus; }
    virtual void focus() = 0;
    virtual void unFocus() = 0;
    virtual int getId() const = 0;
};

#endif
