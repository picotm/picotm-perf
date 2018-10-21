/*
 * picotm-perf - Picotm Performance Tests
 * Copyright (c) 2017   Thomas Zimmermann <contact@tzimmermann.org>
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
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#pragma once

enum parse_opts_result {
    PARSE_OPTS_OK,
    PARSE_OPTS_EXIT,
    PARSE_OPTS_ERROR
};

enum opt_io_pattern {
    IO_PATTERN_RANDOM,
    IO_PATTERN_SEQUENTIAL
};

extern enum opt_io_pattern g_io_pattern;
extern unsigned long       g_nthreads;
extern unsigned long       g_nloads;
extern unsigned long       g_nstores;
extern unsigned long       g_nmsecs;

enum parse_opts_result
parse_opts(int argc, char* argv[]);
