// Copyright (C) 2023 Jacek Lewański
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program. If not, see <https://www.gnu.org/licenses/>.
#include "panel_schema.hpp"
float xOffset = 10.16;                                                              // 2 HP
float yOffset = 14.25;                                                              // top and bottom margin
float xCoords(unsigned char column) { return xOffset + (xOffset * (column << 1)); } // each column has 4 HP
float yCoords(unsigned char row) { return yOffset + (20.f * row); }                 // equal distribution of 6 rows (every 2 cm)