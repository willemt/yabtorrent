#$CC byte_reader.c alltests.c test_byte_reader.c CuTest.c CuTest.h -o test_byte_reader 
#./test_byte_reader

CC=colorgcc
PROGSRC="bt.c bt_client.c bt_piece_db.c bt_filelist.c bt_bitfield.c bt_piece.c byte_reader.c raprogress.c bencode.c bt_metafile_reader.c bt_tracker_response_reader.c url_encoder.c http_request.c bt_peer_connection.c readfile.c sha1.c sha1.h hashmap.c"

#-------------------------------------------------------------------------------
sh make-tests.sh test_bt.c > tests_bt.c
$CC test_bt.c tests_bt.c $PROGSRC CuTest.c CuTest.h -g -o test_bt
./test_bt
echo "\n\n"
#-------------------------------------------------------------------------------
sh make-tests.sh test_peer_connection.c > tests_peer_connection.c
$CC test_peer_connection.c tests_peer_connection.c $PROGSRC CuTest.c CuTest.h -g -o test_btpeerconnection
./test_btpeerconnection
echo "\n\n"
#-------------------------------------------------------------------------------
sh make-tests.sh test_bitfield.c > tests_bitfield.c
$CC test_bitfield.c tests_bitfield.c $PROGSRC CuTest.c CuTest.h -g -o test_bitfield
./test_bitfield
echo "\n\n"
#-------------------------------------------------------------------------------
sh make-tests.sh test_piece_db.c > tests_piece_db.c
$CC test_piece_db.c tests_piece_db.c $PROGSRC CuTest.c CuTest.h -g -o test_piece_db
./test_piece_db
echo "\n\n"
#-------------------------------------------------------------------------------
sh make-tests.sh test_piece.c > tests_piece.c
$CC test_piece.c tests_piece.c $PROGSRC CuTest.c CuTest.h -g -o test_piece
./test_piece
echo "\n\n"
#-------------------------------------------------------------------------------
# Test Random Access progress bar
sh make-tests.sh test_raprogress.c > alltests_raprogress.c
$CC raprogress.c alltests_raprogress.c test_raprogress.c CuTest.c CuTest.h -o test_raprogress 
./test_raprogress
echo "\n\n"
#-------------------------------------------------------------------------------
#$CC bt_metafile_reader.c test_metafile.c tbl.c tbl.h -o test_metafile
#./test_metafile
#-------------------------------------------------------------------------------
# Test bencode reading
sh make-tests.sh test_bencode.c > alltests_bencode.c
$CC bencode.c test_bencode.c alltests_bencode.c CuTest.c CuTest.h -g -o test_metafile
./test_metafile
echo "\n\n"
#-------------------------------------------------------------------------------
# Test Metainfo reading
#$CC bencode.c test_metafile.c bt_metafile_reader.c url_encoder.c sha1.c sha1.h CuTest.c CuTest.h -g -o test_metafile2
#./test_metafile2
#-------------------------------------------------------------------------------

$CC $PROGSRC bt_main.c config.c list.c -g -o bt
./bt

