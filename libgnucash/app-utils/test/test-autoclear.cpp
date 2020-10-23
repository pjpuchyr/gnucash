/********************************************************************
 * test-autoclear.c: test suite for Auto-Clear          	    *
 * Copyright 2020 Cristian Klein <cristian@kleinlabs.eu>            *
 *                                                                  *
 * This program is free software; you can redistribute it and/or    *
 * modify it under the terms of the GNU General Public License as   *
 * published by the Free Software Foundation; either version 2 of   *
 * the License, or (at your option) any later version.              *
 *                                                                  *
 * This program is distributed in the hope that it will be useful,  *
 * but WITHOUT ANY WARRANTY; without even the implied warranty of   *
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the    *
 * GNU General Public License for more details.                     *
 *                                                                  *
 * You should have received a copy of the GNU General Public License*
 * along with this program; if not, you can retrieve it from        *
 * https://www.gnu.org/licenses/old-licenses/gpl-2.0.html            *
 * or contact:                                                      *
 *                                                                  *
 * Free Software Foundation           Voice:  +1-617-542-5942       *
 * 51 Franklin Street, Fifth Floor    Fax:    +1-617-542-2652       *
 * Boston, MA  02110-1301,  USA       gnu@gnu.org                   *
 ********************************************************************/
#include "config.h"

// GoogleTest is written in C++, however, the function we test in C.
extern "C" {
#include "../gnc-ui-balances.h"
}
#include "Split.h"

#include "gtest/gtest.h"

const gint64 DENOM = 100; //< Denomerator is always 100 for simplicity.

struct SplitDatum {
    const char *memo;
    gint64 amount; //< Numerator of amount.
    bool cleared;
};

SplitDatum splitData[] = {
    { "Memo 01", -  8234, true },
    { "Memo 02", -156326, true },
    { "Memo 03", -  4500, true },
    { "Memo 04", -694056, true },
    { "Memo 05", -  7358, true },
    { "Memo 06", - 11700, true },
    { "Memo 07", - 20497, true },
    { "Memo 08", - 11900, true },
    { "Memo 09", -  8275, true },
    { "Memo 10", - 58700, true },
    { "Memo 11", +100000, true },
    { "Memo 12", - 13881, true },
    { "Memo 13", -  5000, true },
    { "Memo 14", +200000, true },
    { "Memo 15", - 16800, true },
    { "Memo 16", -152000, true },
    { "Memo 17", +160000, false },
    { "Memo 18", - 63610, false },
    { "Memo 19", -  2700, false },
    { "Memo 20", - 15400, false },
    { "Memo 21", -  3900, false },
    { "Memo 22", - 22042, false },
    { "Memo 23", -  2900, false },
    { "Memo 24", - 10900, false },
    { "Memo 25", - 44400, false },
    { "Memo 26", -  9200, false },
    { "Memo 27", -  7900, false },
    { "Memo 28", -  1990, false },
    { "Memo 29", -  7900, false },
    { "Memo 30", - 61200, false },
};

struct TestCase {
    gint64 amount;
    const char *expectedErr;
};

TestCase testCases[] = {
    { 0, "The selected amount cannot be cleared.", },
    { -869227, "Account is already at Auto-Clear Balance." }, // No splits need to be cleared.
    { -877127, "Cannot uniquely clear splits. Found multiple possibilities." }, // Two splits need to be cleared.
    { -891269, NULL }, // One split need to be cleared.
    { -963269, NULL }, // All splits need to be cleared.
};

TEST(AutoClear, AutoClearAll) {
    QofBook *book = qof_book_new ();
    Account *account = xaccMallocAccount(book);
    xaccAccountSetName(account, "Test Account");

    for (auto &d : splitData) {
        Split *split = xaccMallocSplit(book);
        xaccSplitSetMemo(split, d.memo);
        xaccSplitSetAmount(split, gnc_numeric_create(d.amount, DENOM));
        xaccSplitSetReconcile(split, d.cleared ? CREC : NREC);
        xaccSplitSetAccount(split, account);

        gnc_account_insert_split(account, split);
    }
    xaccAccountRecomputeBalance(account);

    for (auto &t : testCases) {
        gnc_numeric amount_to_clear = gnc_numeric_create(t.amount, DENOM);
        char *err;

        GList *splits_to_clear = gnc_account_get_autoclear_splits(account, amount_to_clear, &err);

        // Actually clear splits
        for (GList *node = splits_to_clear; node; node = node->next) {
            Split *split = (Split *)node->data;
            xaccSplitSetReconcile(split, CREC);
        }

        ASSERT_STREQ(err, t.expectedErr);
        if (t.expectedErr == NULL) {
            gnc_numeric c = xaccAccountGetClearedBalance(account);
            ASSERT_EQ(c.num, t.amount);
            ASSERT_EQ(c.denom, DENOM);
        }
    }

    qof_book_destroy(book);
}
