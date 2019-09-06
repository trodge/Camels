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
#ifndef ENUM_ARRAY_H
#define ENUM_ARRAY_H

#include <array>

template <typename T, typename EnumType>
class EnumArray : public std::array<T, static_cast<size_t>(EnumType::count)> {
public:
    using Base = std::array<T, static_cast<size_t>(EnumType::count)>;
    using reference = typename Base::reference;
    using const_reference = typename Base::const_reference;
    reference operator[](EnumType pos) { return Base::operator[](static_cast<size_t>(pos)); }
    const_reference operator[](EnumType pos) const { return Base::operator[](static_cast<size_t>(pos)); }
};

#endif // ENUM_ARRAY_H
