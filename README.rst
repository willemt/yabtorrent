YABTorrent
==========
.. image:: https://travis-ci.org/willemt/YABTorrent.png
   :target: https://travis-ci.org/willemt/YABTorrent

What?
-----
Yet another Bittorrent client/library written in C with a BSD license. The client has been intended to be used as a drop-in bittorrent library. The client is in ALPHA and currently has 130+ unit tests covering the code base.

Why?
----
YABTorrent was spawned by:

1. There are not that many bittorrent clients in C that are licensed under non-copyleft licenses

2. There are not that many lightweight torrent libraries which can be used for adding bittorrent-like download capability. 

How does it work?
-----------------

.. image:: http://github.com/willemt/YABTorrent/raw/master/doc/YABTorrent.png

YABTorrent is event based.

**Networkfuncs** is a set of networking functions that need to be implemented for the required plumbing. Currently networkworkfuncs_libuv.c is a libuv implementation.

See bt.h for documentation.

Building
--------

$git clone https://github.com/willemt/YABTorrent

$cd YABTorrent

$python waf configure

$python waf build


Usage
-----

$./yabtorrent torrentfile.torrent

Help/Todo
---------
If you want to help out with development, please look into the following:
- BEP 5 - DHT Protocol - github.com/willemt/CTrackerClient simply needs to handle the "dht://" schema. github.com/jech/dht can be used to add this functionality.
- BEP 2 - uTorrent transport protocol - you need to create a "networkfuncs_utp.c" source file which implements all the network callbacks. "networkfuncs_utp.c" would simply be a wrapper of github.com/bittorrent/libutp.
- BEP 9 - Extension for Peers to Send Metadata Files
- uTorrent Peer Exchange
