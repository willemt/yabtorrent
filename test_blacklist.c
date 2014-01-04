
#include <assert.h>
#include <setjmp.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "CuTest.h"

#include <stdint.h>

#include "bt.h"
#include "bt_local.h"
#include "bt_blacklist.h"

void TestBT_blacklist_after_new_is_empty(
    CuTest * tc
)
{
    void *b;
    
    b = bt_blacklist_new();

    CuAssertTrue(tc, 0 == bt_blacklist_get_npieces(b));
}

void TestBT_blacklist_after_adding_piece_npieces_is_accurate(
    CuTest * tc
)
{
    void *b;
    
    b = bt_blacklist_new();

    bt_blacklist_add_peer(b,(void*)1,(void*)2);
    CuAssertTrue(tc, 1 == bt_blacklist_get_npieces(b));
    bt_blacklist_add_peer(b,(void*)2,(void*)2);
    CuAssertTrue(tc, 2 == bt_blacklist_get_npieces(b));
}

void TestBT_blacklist_after_adding_dupe_piece_npieces_is_accurate(
    CuTest * tc
)
{
    void *b;
    
    b = bt_blacklist_new();

    bt_blacklist_add_peer(b,(void*)1,(void*)2);
    CuAssertTrue(tc, 1 == bt_blacklist_get_npieces(b));
    bt_blacklist_add_peer(b,(void*)1,(void*)2);
    CuAssertTrue(tc, 1 == bt_blacklist_get_npieces(b));
    bt_blacklist_add_peer(b,(void*)1,(void*)3);
    CuAssertTrue(tc, 1 == bt_blacklist_get_npieces(b));
}

void TestBT_blacklist_after_adding_peer_is_recognised_as_blacklisted(
    CuTest * tc
)
{
    void *b;
    
    b = bt_blacklist_new();

    bt_blacklist_add_peer(b,(void*)1,(void*)2);
    CuAssertTrue(tc, 1 == bt_blacklist_peer_is_blacklisted(b,(void*)1,(void*)2));
    CuAssertTrue(tc, 0 == bt_blacklist_peer_is_potentially_blacklisted(b,(void*)1,(void*)2));
}

void TestBT_blacklist_after_adding_peer_as_potentially_is_recognised_as_potentialy_blacklisted(
    CuTest * tc
)
{
    void *b;
    
    b = bt_blacklist_new();

    bt_blacklist_add_peer_as_potentially_blacklisted(b,(void*)1,(void*)2);
    CuAssertTrue(tc, 0 == bt_blacklist_peer_is_blacklisted(b,(void*)1,(void*)2));
    CuAssertTrue(tc, 1 == bt_blacklist_peer_is_potentially_blacklisted(b,(void*)1,(void*)2));
}

