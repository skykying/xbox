/*
 *  Input Interface
 *
 *   This program is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation; either version 2 of the License, or
 *   (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program; if not, write to the Free Software
 *   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307 USA
 *
 * Copyright (C) 2016 EPAM Systems Inc.
 */

#ifndef SRC_INPUTITF_HPP_
#define SRC_INPUTITF_HPP_

#include <exception>
#include <memory>

namespace InputItf {

/***************************************************************************//**
 * @defgroup input_itf Input interface
 * Abstract classes for input devices implementation.
 ******************************************************************************/

struct KeyboardCallbacks
{
	std::function<void(uint32_t key, uint32_t state)> key;
};

struct PointerCallbacks
{
	std::function<void(int32_t x, int32_t y, int32_t relZ)> moveRelative;
	std::function<void(int32_t x, int32_t y, int32_t relZ)> moveAbsolute;
	std::function<void(uint32_t button, uint32_t state)> button;
};

struct TouchCallbacks
{
	std::function<void(int32_t id, int32_t x, int32_t y)> down;
	std::function<void(int32_t id)> up;
	std::function<void(int32_t id, int32_t x, int32_t y)> motion;
	std::function<void(int32_t id)> frame;
};

template<typename T>
class InputDevice
{
public:
	virtual ~InputDevice() {}

	virtual void setCallbacks(const T& callbacks) = 0;
};

typedef InputDevice<KeyboardCallbacks> Keyboard;
typedef InputDevice<PointerCallbacks> Pointer;
typedef InputDevice<TouchCallbacks> Touch;

typedef std::shared_ptr<Keyboard> KeyboardPtr;
typedef std::shared_ptr<Pointer> PointerPtr;
typedef std::shared_ptr<Touch> TouchPtr;

}

#endif /* SRC_INPUTITF_HPP_ */
