

Integration testing
===================
Implement the following tests via a Vagrantfile:

 - Connect to HTTP tracker. Get torrent details
 - Connect to UDP tracker. Get torrent details
 - Download from single peer (Transmission)
 - Download from single peer (uTorrent)
 - Download from single peer (BitComet)
 - Download from two peers (one peer has corrupted files)

Planned features
================

Performance
-----------
Use sendfile() on Linux for zero copy send.
