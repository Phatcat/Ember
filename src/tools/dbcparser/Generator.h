/*
 * Copyright (c) 2014 Ember
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include "Types.h"
#include <vector>
#include <string>

namespace ember { namespace dbc {

void generate_common(const std::vector<types::Definition>& defs, const std::string& output);
void generate_disk_source(const std::vector<types::Definition>& defs, const std::string& output);

}} //dbc, ember