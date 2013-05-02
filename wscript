def options(opt):
        opt.load('compiler_c')


contribs = [
('CBitfield', 'git@github.com:willemt/CBitfield.git'),
('CLinkedListQueue', 'git@github.com:willemt/CLinkedListQueue.git'),
('CBTTrackerClient', 'git@github.com:willemt/CBTTrackerClient.git'),
('CCircularBuffer', 'git@github.com:willemt/CCircularBuffer.git'),
('CSparseCounter', 'git@github.com:willemt/CSparseCounter.git'),
('CBitstream', 'git@github.com:willemt/CSimpleBitstream.git'),
('CConfig-re', 'git@github.com:willemt/CConfig-re.git'),
('CBTPWPConnection', 'git@github.com:willemt/CBTPWPConnection.git'),
('CSparseFileAllocator', 'git@github.com:willemt/CSparseFileAllocator.git'),
('CEventTimer', 'git@github.com:willemt/CEventTimer.git'),
('CHeaplessBencodeReader', 'git@github.com:willemt/CHeaplessBencodeReader.git'),
('CHashMapViaLinkedList','git@github.com:willemt/CHashMapViaLinkedList.git'),
('CMeanQueue','git@github.com:willemt/CMeanQueue.git'),
('CHeap','git@github.com:willemt/CHeap.git'),
('CPSeudoLRU','git@github.com:willemt/CPseudoLRU.git')]

#bld(rule='mkdir clinkedlistqueue && git pull git@github.com:willemt/CLinkedListQueue.git', always=True)

def configure(conf):
    conf.load('compiler_c')

    # Get the required contributions via GIT
    for c in contribs:
        print "Git pulling %s..." % c[1]
        #conf.exec_command("mkdir %s" % c[0])
        #conf.exec_command("git init", cwd=c[0])
        #conf.exec_command("git pull %s" % c[1], cwd=c[0])
        conf.exec_command("git clone %s" % "../"+c[0])


from waflib.Task import Task
class compiletest(Task):
        def run(self):
                return self.exec_command('sh ../make-tests.sh %s > %s' % (
                                self.inputs[0].abspath(),
                                self.outputs[0].abspath()
                        )
                )

def unittest(bld, src):
        bld(rule='sh ../make-tests.sh ${SRC} > ${TGT}', source=src, target="t"+src)
        bld.program(
                source='%s %s CuTest.c' % (src,"t"+src),
                target=src[:-2],
                cflags=[
                    '-g',
                    '-Werror'
                ],
                use='yabbt',
                unit_test='yes',
                includes='.')

        bld(rule='pwd && export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:. && ./${SRC}', source=src[:-2])
#        bld(rule='export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:. && ./${SRC}', source=src[:-2])#, 'tests_'+src)


#def shutdown(bld):
#    pass
#    from waflib.UnitTest import UnitTest
#    unittest = UnitTest.unit_test()
#    unittest.run ()
#    unittest.print_results()

def build(bld):

        contrib_dir = '../'

        import sys

        if sys.platform == 'win32':
            platform = '-D__WINDOWS__'
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
                    "bt_networkfuncs_mock.c",
                    "readfile.c",
                    "sha1.c",
                    "bt_selector_rarestfirst.c",
                    contrib_dir+"CHeap/heap.c",
                    contrib_dir+"CHeaplessBencodeReader/bencode.c",
                    contrib_dir+"CHashMapViaLinkedList/linked_list_hashmap.c",
                    contrib_dir+"CLinkedListQueue/linked_list_queue.c",
                    contrib_dir+"CBitstream/bitstream.c",
                    contrib_dir+"CBTPWPConnection/pwp_connection.c",
                    contrib_dir+"CSparseFileAllocator/sparsefile_allocator.c",
                    contrib_dir+"CSparseCounter/sparse_counter.c",
                    contrib_dir+"CPSeudoLRU/pseudolru.c",
                    contrib_dir+"CEventTimer/event_timer.c",
                    contrib_dir+"CBitfield/bitfield.c",
                    contrib_dir+"CConfig-re/list.c",
                    contrib_dir+"CConfig-re/config.c",
                    ],
                #bt_diskmem.c
                #CCircularBuffer/cbuffer.c
                #use='config',
                target='yabbt',
                includes=[
                    contrib_dir+"CHeaplessBencodeReader",
                    contrib_dir+"CLinkedListQueue",
                    contrib_dir+"CHashMapViaLinkedList",
                    contrib_dir+"CHeap",
                    contrib_dir+"CPSeudoLRU",
                    contrib_dir+"CCircularBuffer",
                    contrib_dir+"CEventTimer",
                    contrib_dir+"CSparseCounter",
                    contrib_dir+"CSparseFileAllocator",
                    contrib_dir+"CBitfield",
                    contrib_dir+"CBTPWPConnection",
                    contrib_dir+"CBitstream",
                    contrib_dir+"CConfig-re",
                    contrib_dir+"CBTTrackerClient"
                   ], 
                cflags=[
                    '-Werror',
                    '-g',
                    platform,
                    '-Werror=uninitialized',
                    '-Werror=return-type',
                    '-Wcast-align'],
                )

#                use='config bencode',
#        bld(rule='../tests_bt', target='tests_bt.c')

#        unittest(bld,'test_bt.c')
#        unittest(bld,'test_bitfield.c')
#        unittest(bld,'test_bt.c')
#        unittest(bld,'test_byte_reader.c')
#        unittest(bld,'test_filedumper.c')
##        unittest(bld,'test_metafile.c')
#        unittest(bld,'test_piece.c')
#        unittest(bld,'test_piece_db.c')
##        unittest(bld,'test_pwp_event_manager.c')
#        unittest(bld,'test_peer_connection.c')
#        unittest(bld,'test_choker_leecher.c')
#        unittest(bld,'test_choker_seeder.c')
#        unittest(bld,'test_rarestfirst.c')
##        unittest(bld,'test_peer_connection_read.c')
##        unittest(bld,'test_peer_connection_send.c')

        bld.program(
                source=[
                    'bt_main.c',
                    contrib_dir+"CBTTrackerClient/bt_tracker_client.c",
                    contrib_dir+"CBTTrackerClient/bt_tracker_client_response_reader.c",
                    contrib_dir+"CBTTrackerClient/url_encoder.c"
                    ],
                target='bt',
                cflags=[
                    '-g',
                    '-Werror',
                    '-Werror=uninitialized',
                    '-Werror=return-type'
                    ],
                includes=[
                    contrib_dir+"CConfig-re",
                    contrib_dir+"CBTTrackerClient",
                    contrib_dir+"CHeaplessBencodeReader",
                   ], 
                use='yabbt')



