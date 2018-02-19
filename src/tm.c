/* Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
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
