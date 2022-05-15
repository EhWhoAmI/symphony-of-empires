// Symphony of Empires
// Copyright (C) 2021, Symphony of Empires contributors
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 2 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License along
// with this program; if not, write to the Free Software Foundation, Inc.,
// 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
//
// ----------------------------------------------------------------------------
// Name:
//      eng3d/ui/numeric_input.hpp
//
// Abstract:
//      Numeric input allows the user to input only numerical digits
//      useful for dates or decimal digits.
// ----------------------------------------------------------------------------

#pragma once

#include <string>
#include "eng3d/ui/group.hpp"

namespace UI {
	class Context;
    class Input;
    class Button;
    
    /**
     * @ingroup UI
     * @brief A version of the Input widget that only allows numbers.
     *
     */
	class NumericInput : public Group {
	public:
		NumericInput(int _x, int _y, unsigned w, unsigned h, Widget* _parent);
		void set_buffer(const std::string& _buffer);
		std::string get_buffer(void) const;

        Input* inp;
        Button* add_btn;
        Button* sub_btn;
	};
};