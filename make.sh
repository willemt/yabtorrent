#gcc byte_reader.c alltests.c test_byte_reader.c CuTest.c CuTest.h -o test_byte_reader 

#./test_byte_reader

sh make-tests.sh test_bt.c > tests_bt.c

gcc test_bt.c tests_bt.c bt.c bencode.c bt_metafile_reader.c bt_tracker_response_reader.c url_encoder.c http_request.c readfile.c sha1.c sha1.h CuTest.c CuTest.h -g -o test_bt

./test_bt

gcc bt.c bt_main.c bencode.c bt_metafile_reader.c bt_tracker_response_reader.c url_encoder.c readfile.c sha1.c sha1.h CuTest.c CuTest.h -g -o bt

./bt


#gcc bt_metafile_reader.c test_metafile.c tbl.c tbl.h -o test_metafile

#./test_metafile
