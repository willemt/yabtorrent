import sys, os

def options(opt):
        opt.load('compiler_c')


contribs = [
('CBitfield', 'http://github.com/willemt/CBitfield.git'),
('CLinkedListQueue', 'http://github.com/willemt/CLinkedListQueue.git'),
('CBTTrackerClient', 'http://github.com/willemt/CBTTrackerClient.git'),
('CBitstream', 'http://github.com/willemt/CSimpleBitstream.git'),
('CTorrentFileReader', 'http://github.com/willemt/CTorrentFileReader.git'),
('CCircularBuffer', 'http://github.com/willemt/CCircularBuffer.git'),
('CSparseCounter', 'http://github.com/willemt/CSparseCounter.git'),
('CBitstream', 'http://github.com/willemt/CSimpleBitstream.git'),
('CConfig-re', 'http://github.com/willemt/CConfig-re.git'),
('CBTPWPConnection', 'http://github.com/willemt/CBTPWPConnection.git'),
('CSparseFileAllocator', 'http://github.com/willemt/CSparseFileAllocator.git'),
('CEventTimer', 'http://github.com/willemt/CEventTimer.git'),
('CHeaplessBencodeReader', 'http://github.com/willemt/CHeaplessBencodeReader.git'),
('CHashMapViaLinkedList', 'http://github.com/willemt/CHashMapViaLinkedList.git'),
('CMeanQueue', 'http://github.com/willemt/CMeanQueue.git'),
('CHeap', 'http://github.com/willemt/CHeap.git'),
('CPSeudoLRU', 'http://github.com/willemt/CPseudoLRU.git')]

#bld(rule='mkdir clinkedlistqueue && git pull git@github.com:willemt/CLinkedListQueue.git', always=True)

def configure(conf):
    conf.load('compiler_c')

    # Get the required contributions via GIT
    for c in contribs:
        print "Pulling via git %s..." % c[1]
        #conf.exec_command("mkdir %s" % c[0])
        #conf.exec_command("git init", cwd=c[0])
        #conf.exec_command("git pull %s" % c[1], cwd=c[0])
        if not os.path.exists("../"+c[0]):
#            conf.exec_command("git clone %s" % "../"+c[0])
            conf.env.CONTRIB_PATH = './'
            conf.exec_command("git clone %s %s" % (c[1],c[0],))
        else:
            conf.env.CONTRIB_PATH = '../../'
            


from waflib.Task import Task

class compiletest(Task):
        def run(self):
                return self.exec_command('sh ../make-tests.sh %s > %s' % (
                                self.inputs[0].abspath(),
                                self.outputs[0].abspath()
                        )
                )

def unittest(bld, src, ccflag=None):
        bld(rule='cp ../make-tests.sh .')
        bld(rule='cp ../%s .' % src)
        # collect tests into one area
        bld(rule='sh make-tests.sh '+src+' > ${TGT}', target="t_"+src)


        # build the test program
        bld.program(
                source=[
                    src,
                    "t_"+src,
                    'CuTest.c',
                    bld.env.CONTRIB_PATH+"CBitfield/bitfield.c",
                ],
                target=src[:-2],
                cflags=[
                    '-g',
                    '-Werror'
                ],
                use='yabbt',
                unit_test='yes',
                includes=[
                    '.',
                    bld.env.CONTRIB_PATH+"CBitfield"
                ]
                )

        # run the test
        if sys.platform == 'win32':
            bld(rule='${SRC}',source=src[:-2]+'.exe')
        else:
            bld(rule='pwd && export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:. && ./'+src[:-2])


#def shutdown(bld):
#    pass
#    from waflib.UnitTest import UnitTest
#    unittest = UnitTest.unit_test()
#    unittest.run ()
#    unittest.print_results()

def build(bld):

        if sys.platform == 'win32':
            platform = '-D__WINDOWS__'
	elif sys.platform == 'linux2':
            platform = '-D__LINUX__'
        else:
            platform = ''

        bld.shlib(
                source= [
                    "bt_client.c",
                    "bt_client_properties.c",
                    "bt_peermanager.c",
                    "bt_sha1.c",
                    "bt_util.c",
                    "bt_piece_db.c",
                    "bt_filedumper.c",
                    "bt_piece.c",
                    "bt_choker_leecher.c",
                    "bt_choker_seeder.c",
                    "bt_diskcache.c",
                    "bt_string.c",
                    "bt_diskmem.c",
                    "readfile.c",
                    "sha1.c",
                    "bt_selector_rarestfirst.c",
                    bld.env.CONTRIB_PATH+"CHeap/heap.c",
                    bld.env.CONTRIB_PATH+"CHeaplessBencodeReader/bencode.c",
                    bld.env.CONTRIB_PATH+"CHashMapViaLinkedList/linked_list_hashmap.c",
                    bld.env.CONTRIB_PATH+"CLinkedListQueue/linked_list_queue.c",
                    bld.env.CONTRIB_PATH+"CBitstream/bitstream.c",
                    bld.env.CONTRIB_PATH+"CBTPWPConnection/pwp_connection.c",
                    bld.env.CONTRIB_PATH+"CSparseFileAllocator/sparsefile_allocator.c",
                    bld.env.CONTRIB_PATH+"CSparseCounter/sparse_counter.c",
                    bld.env.CONTRIB_PATH+"CPSeudoLRU/pseudolru.c",
                    bld.env.CONTRIB_PATH+"CEventTimer/event_timer.c",
                    bld.env.CONTRIB_PATH+"CBitfield/bitfield.c",
                    bld.env.CONTRIB_PATH+"CConfig-re/list.c",
                    bld.env.CONTRIB_PATH+"CConfig-re/config.c",
                    bld.env.CONTRIB_PATH+"CTorrentFileReader/torrentfile_reader.c",
                    ],
                #bt_diskmem.c
                #CCircularBuffer/cbuffer.c
                #use='config',
                target='yabbt',
                includes=[
                    bld.env.CONTRIB_PATH+"CHeaplessBencodeReader",
                    bld.env.CONTRIB_PATH+"CLinkedListQueue",
                    bld.env.CONTRIB_PATH+"CHashMapViaLinkedList",
                    bld.env.CONTRIB_PATH+"CHeap",
                    bld.env.CONTRIB_PATH+"CPSeudoLRU",
                    bld.env.CONTRIB_PATH+"CCircularBuffer",
                    bld.env.CONTRIB_PATH+"CEventTimer",
                    bld.env.CONTRIB_PATH+"CSparseCounter",
                    bld.env.CONTRIB_PATH+"CSparseFileAllocator",
                    bld.env.CONTRIB_PATH+"CBitfield",
                    bld.env.CONTRIB_PATH+"CBTPWPConnection",
                    bld.env.CONTRIB_PATH+"CBitstream",
                    bld.env.CONTRIB_PATH+"CConfig-re",
                    bld.env.CONTRIB_PATH+"CBTTrackerClient",
                   ], 
                cflags=[
                    '-Werror',
                    '-g',
                    platform,
                    '-Werror=uninitialized',
                    '-Werror=return-type',
                    '-Wcast-align'],
                )

        unittest(bld,"test_bt.c")
        unittest(bld,"test_peermanager.c")
        unittest(bld,'test_choker_leecher.c')
        unittest(bld,'test_choker_seeder.c')
        unittest(bld,'test_rarestfirst.c')
        unittest(bld,'test_filedumper.c')
        unittest(bld,'test_piece.c',ccflag='-I../'+bld.env.CONTRIB_PATH+"CBitfield")
        unittest(bld,'test_piece_db.c')

        bld.program(
                source=[
                    'bt_main.c',
                    "networkfuncs_mock.c",
                    bld.env.CONTRIB_PATH+"CBTTrackerClient/bt_tracker_client.c",
                    bld.env.CONTRIB_PATH+"CBTTrackerClient/bt_tracker_client_response_reader.c",
                    bld.env.CONTRIB_PATH+"CBTTrackerClient/url_encoder.c",
                    bld.env.CONTRIB_PATH+"CCircularBuffer/cbuffer.c"
                    ],
                target='bt',
                cflags=[
                    '-g',
                    '-Werror',
                    '-Werror=uninitialized',
                    '-Werror=return-type'
                    ],
                includes=[
                    bld.env.CONTRIB_PATH+"CConfig-re",
                    bld.env.CONTRIB_PATH+"CBTTrackerClient",
                    bld.env.CONTRIB_PATH+"CHeaplessBencodeReader",
                    bld.env.CONTRIB_PATH+"CTorrentFileReader",
                    bld.env.CONTRIB_PATH+"CHashMapViaLinkedList",
                    bld.env.CONTRIB_PATH+"CCircularBuffer",
                   ], 
                use='yabbt')





