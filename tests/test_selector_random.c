
#include <stdbool.h>
#include <assert.h>
#include <setjmp.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "CuTest.h"

#include <stdint.h>

#include "bt.h"
#include "bt_selector_random.h"

static bt_pieceselector_i iface = {
    .new = bt_random_selector_new,
    .peer_giveback_piece = bt_random_selector_giveback_piece,
    .have_piece = bt_random_selector_have_piece,
    .remove_peer = bt_random_selector_remove_peer,
    .add_peer = bt_random_selector_add_peer,
    .peer_have_piece = bt_random_selector_peer_have_piece,
    .get_npeers = bt_random_selector_get_npeers,
    .get_npieces = bt_random_selector_get_npieces,
    .poll_piece = bt_random_selector_poll_best_piece
};

void TestSelectorRandom_new_is_initialised_with_npieces(
    CuTest * tc
)
{
    void *cr;

    cr = iface.new(10);
    CuAssertTrue(tc, 10 == iface.get_npieces(cr));
}

void TestSelectorRandom_add_peer_adds_peer(
    CuTest * tc
)
{
    void *cr;

    cr = iface.new(10);
    CuAssertTrue(tc, 0 == iface.get_npeers(cr));

    iface.add_peer(cr, (void *) 1);
    CuAssertTrue(tc, 1 == iface.get_npeers(cr));
}

void TestSelectorRandom_remove_peer_removes_peer(
    CuTest * tc
)
{
    void *cr;

    cr = iface.new(10);
    CuAssertTrue(tc, 0 == iface.get_npeers(cr));

    iface.add_peer(cr, (void *) 1);
    iface.remove_peer(cr, (void *) 1);
    CuAssertTrue(tc, 0 == iface.get_npeers(cr));
}

void TestSelectorRandom_cant_poll_with_no_haves(
    CuTest * tc
)
{
    void *cr;

    cr = iface.new(10);
    iface.add_peer(cr, (void *) 1);
    CuAssertTrue(tc, -1 == iface.poll_piece(cr, (void *) 1));
}

void TestSelectorRandom_poll_from_peer(
    CuTest * tc
)
{
    void *cr;

    cr = iface.new(10);
    iface.add_peer(cr, (void *) 1);
    iface.peer_have_piece(cr, (void *) 1, 1);
    /*  get piece 1 from peer */
    CuAssertTrue(tc, 1 == iface.poll_piece(cr, (void *) 1));
}

void TestSelectorRandom_cant_poll_piece_again(
    CuTest * tc
)
{
    void *cr;
    int piece;

    cr = iface.new(10);
    iface.add_peer(cr, (void *) 1);
    iface.add_peer(cr, (void *) 2);
    iface.add_peer(cr, (void *) 3);
    iface.peer_have_piece(cr, (void *) 1, 1);
    iface.peer_have_piece(cr, (void *) 2, 1);
    iface.peer_have_piece(cr, (void *) 3, 1);
    /*  this is the rarest */
    CuAssertTrue(tc, 1 == iface.poll_piece(cr, (void *) 3));
    CuAssertTrue(tc, -1 == iface.poll_piece(cr, (void *) 3));
}

void TestSelectorRandom_poll_piece_again_after_adding_back(
    CuTest * tc
)
{
    void *cr;

    cr = iface.new(10);
    iface.add_peer(cr, (void *) 1);
    iface.add_peer(cr, (void *) 2);
    iface.add_peer(cr, (void *) 3);
    iface.peer_have_piece(cr, (void *) 1, 1);
    iface.peer_have_piece(cr, (void *) 2, 1);
    iface.peer_have_piece(cr, (void *) 3, 1);
    CuAssertTrue(tc, 1 == iface.poll_piece(cr, (void *) 3));
    iface.peer_giveback_piece(cr, (void*)3, 1);
    CuAssertTrue(tc, 1 == iface.poll_piece(cr, (void *) 3));
}

void TestSelectorRandom_have_removes_piece_from_polling(
    CuTest * tc
)
{
    void *cr;

    cr = iface.new(10);
    iface.add_peer(cr, (void *) 1);
    iface.add_peer(cr, (void *) 2);
    iface.add_peer(cr, (void *) 3);
    iface.peer_have_piece(cr, (void *) 1, 1);
    iface.peer_have_piece(cr, (void *) 2, 1);
    iface.peer_have_piece(cr, (void *) 3, 1);
    iface.peer_have_piece(cr, (void *) 3, 2);
    /*  we now have this piece.. */
    iface.have_piece(cr, 2);
    /*  ..which means we should poll it. */
    CuAssertTrue(tc, 1 == iface.poll_piece(cr, (void *) 3));
    CuAssertTrue(tc, -1 == iface.poll_piece(cr, (void *) 3));
}
