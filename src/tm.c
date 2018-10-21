/*
 * picotm-perf - Picotm Performance Tests
 * Copyright (c) 2017-2018  Thomas Zimmermann <contact@tzimmermann.org>
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

#include "tm.h"
#include <picotm/picotm.h>
#include <picotm/picotm-tm-ctypes.h>
#include <picotm/stdlib-tm.h>
#include "ptr.h"
#include "testhlp.h"

uint8_t mem_buf[1024];

void
tm_test_random_rw(unsigned long tid, unsigned long nloads, unsigned long nstores)
{
    picotm_begin

        unsigned int seed = tid;

        for (unsigned long i = 0; i < nloads; ++i) {

            int rngval = rand_r_tm(&seed);

            unsigned long off = rngval % sizeof(mem_buf);

            load_ulong_tx((void*)(mem_buf + off));
        }

        for (unsigned long i = 0; i < nstores; ++i) {

            int rngval = rand_r_tm(&seed);

            unsigned long off = rngval % sizeof(mem_buf);

            store_ulong_tx((void*)(mem_buf + off), tid);
        }

    picotm_commit

        abort_transaction_on_error(__func__);

    picotm_end
}

void
tm_test_seq_rw(unsigned long tid, unsigned long nloads, unsigned long nstores)
{
    unsigned int seed = tid;
    int rngval = rand_r(&seed);

    picotm_begin

        unsigned long off = rngval;

        for (unsigned long i = 0; i < nloads; ++i, ++off) {

            off %= sizeof(mem_buf);

            load_ulong_tx((void*)(mem_buf + off));
        }

        for (unsigned long i = 0; i < nstores; ++i, ++off) {

            off %= sizeof(mem_buf);

            store_ulong_tx((void*)(mem_buf + off), tid);
        }

    picotm_commit

        abort_transaction_on_error(__func__);

    picotm_end
}

struct test_func tm_test[] = {
    {
        "random_rw",
        tm_test_random_rw
    },
    {
        "seq_rw",
        tm_test_seq_rw
    }
};

size_t
number_of_tm_tests()
{
    return arraylen(tm_test);
}
