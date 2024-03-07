/* core.h
 * Copyright (C) 2023  Frederick Valdez
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#ifndef CORE_H
#define CORE_H

#include <iostream>
#include <boost/filesystem.hpp>
#include "pid.h"
#include "process_file.h"

/* define term color style */
#define TERM_COLOR_DEFAULT "\033[0m"
#define TERM_COLOR_RED "\033[31;1m"
#define TERM_COLOR_GREEN "\033[32;2m"
/* end color */

#define RETURN_CODE_SUCESS 1
#define RETURN_CODE_FAILED -1

struct home_dir
{
	std::string cwd = boost::filesystem::current_path().generic_string();
};

void msg_err(const char *arg, int code);
void msg_print(std::string arg);

void found_file(const std::string &path);
const char *extract_path(const char *p);
int buffer_open(const std::string &e);
int start(const std::string &path);
#endif // CORE_H
