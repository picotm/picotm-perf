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

#include <stdio.h>
#include <stdlib.h>
#include "test.h"
#include "tm.h"
#include "opts.h"

static const struct test_func*
find_test(enum opt_io_pattern io_pattern)
{
    if (io_pattern >= number_of_tm_tests()) {
        fprintf(stderr, "no test for given I/O pattern\n");
        return NULL;
    }
    return tm_test + io_pattern;
}

int
main(int argc, char* argv[])
{
    switch (parse_opts(argc, argv)) {
        case PARSE_OPTS_EXIT:
            return EXIT_SUCCESS;
        case PARSE_OPTS_ERROR:
            return EXIT_FAILURE;
        default:
            break;
    }

    const struct test_func* test = find_test(g_io_pattern);
    if (!test) {
        return EXIT_FAILURE;
    }

    int res = run_test(g_nthreads, g_nmsecs, test, g_nloads, g_nstores);
    if (res < 0) {
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
