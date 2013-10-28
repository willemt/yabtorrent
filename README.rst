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

2. There are not that many light weight torrent libraries which can be used for adding bittorrent-like download capability. 

Building
--------

$git clone https://www.github.com/willemt/YABTorrent

$cd YABTorrent

$python waf configure

$python waf build


Usage
-----

$./yabtorrent torrentfile.torrent
