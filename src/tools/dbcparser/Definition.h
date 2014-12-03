/*
 * Copyright (c) 2014 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include "Field.h"
#include <string>
#include <vector>

namespace ember { namespace dbc {

struct Definition {
	std::string dbc_name;
	std::string alias;
	std::vector<Field> fields;
};

}} //dbc, ember