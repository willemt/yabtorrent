import sys, os

def options(opt):
        opt.load('compiler_c')

contribs = [
('CBag', 'http://github.com/willemt/CBag'),
('CHeap','http://github.com/willemt/CHeap'),
('libuv','http://github.com/joyent/libuv'),
('CBitfield', 'http://github.com/willemt/CBitfield'),
('CPSeudoLRU','http://github.com/willemt/CPseudoLRU'),
('CMeanQueue','http://github.com/willemt/CMeanQueue'),
('CBipBuffer', 'http://github.com/willemt/CBipBuffer'),
('CConfig-re', 'http://github.com/willemt/CConfig-re'),
('CEventTimer', 'http://github.com/willemt/CEventTimer'),
('CSparseCounter', 'http://github.com/willemt/CSparseCounter'),
('CLinkedListQueue', 'http://github.com/willemt/CLinkedListQueue'),
('CBTTrackerClient', 'http://github.com/willemt/CBTTrackerClient'),
('CSimpleBitstream', 'http://github.com/willemt/CSimpleBitstream'),
('CBTPWPConnection', 'http://github.com/willemt/CBTPWPConnection'),
('CTorrentFileReader', 'http://github.com/willemt/CTorrentFileReader'),
('CSparseFileAllocator', 'http://github.com/willemt/CSparseFileAllocator'),
('CHashMapViaLinkedList','http://github.com/willemt/CHashMapViaLinkedList'),
('CHeaplessBencodeReader', 'http://github.com/willemt/CHeaplessBencodeReader'),
]

def configure(conf):
    conf.load('compiler_c')
    if sys.platform == 'win32':
        conf.check_cc(lib='ws2_32')
        conf.check_cc(lib='psapi')

    conf.env.STDLIBPATH = ['.']
#    conf.check_cc(lib='uv')

    conf.find_program("git")

    # Get the required contributions via GIT
    for c in contribs:
        print "Pulling via git %s..." % c[1]
        #conf.exec_command("mkdir %s" % c[0])
        #conf.exec_command("git init", cwd=c[0])
        #conf.exec_command("git pull %s" % c[1], cwd=c[0])
        if not os.path.exists("../"+c[0]):
            conf.env.CONTRIB_PATH = './'
            conf.exec_command("git clone %s %s" % (c[1],c[0],))
            conf.exec_command("git pull %s" % c[1], cwd=c[0])
        else:
            conf.env.CONTRIB_PATH = '../'
            
    print "Configuring libuv (autogen.sh)"
    conf.exec_command("sh autogen.sh", cwd="libuv")
    print "Configuring libuv (configure)"
    conf.exec_command("sh configure", cwd="libuv")
    print "Building libuv.a"
    conf.exec_command("make", cwd="libuv")
    conf.exec_command("mkdir build")
    conf.exec_command("cp libuv/.libs/libuv.a build/")


from waflib.Task import Task

class compiletest(Task):
        def run(self):
                return self.exec_command('sh ../make-tests.sh %s > %s' % (
                                self.inputs[0].abspath(),
                                self.outputs[0].abspath()
                        )
                )

def unittest(bld, src, ccflag=None):
        #bld(rule='cp ../make-tests.sh .')
        #bld(rule='cp ../%s .' % src)
        # collect tests into one area
        bld(rule='sh ../make-tests.sh ../'+src+' > ${TGT}', target="t_"+src)

        libs = []

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
                lib = libs,
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
            bld(rule='export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:. && ./${SRC}',source=src[:-2])
            #bld(rule='pwd && ./build/'+src[:-2])

def end2end(bld, src, ccflag=None):
    bld(rule='cp ../make-tests.sh .')
    bld(rule='cp ../%s .' % src)
    # collect tests into one area
    bld(rule='sh ../make-tests.sh '+src+' > ${TGT}', target="t_"+src)

    libs = []
    bld.program(
        source=[
            src,
            "t_"+src,
            'CuTest.c',
            "networkfuncs_mock.c",
            "mt19937ar.c",
            "mock_torrent.c",
            bld.env.CONTRIB_PATH+"CBipBuffer/bipbuffer.c"
            ],
        stlibpath = ['libuv','.'],
        target='test_end_to_end',
        cflags=[
            '-g',
            '-Werror',
            '-Werror=uninitialized',
            '-Werror=return-type'
            ],
        lib = libs,
        unit_test='yes',
        includes=[
            bld.env.CONTRIB_PATH+"CConfig-re",
            bld.env.CONTRIB_PATH+"CBTTrackerClient",
            bld.env.CONTRIB_PATH+"CHeaplessBencodeReader",
            bld.env.CONTRIB_PATH+"CTorrentFileReader",
            bld.env.CONTRIB_PATH+"CHashMapViaLinkedList",
            bld.env.CONTRIB_PATH+"CBipBuffer",
           ], 
        use='yabbt')

    # run the test
    if sys.platform == 'win32':
        bld(rule='${SRC}',source=src[:-2]+'.exe')
    else:
        bld(rule='export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:. && ./${SRC}',source=src[:-2])
        #bld(rule='pwd && export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:. && ./'+src[:-2])

def build(bld):
    cp = bld.env.CONTRIB_PATH

    # Copy libuv.a to build/
    bld(rule='cp ../libuv/.libs/libuv.a .', always=True)#, target="libuv.a")

    if sys.platform == 'win32':
        platform = '-DWIN32'
    elif sys.platform == 'linux2':
        platform = '-DLINUX'
    else:
        platform = ''

    libs = []

    bld.shlib(
        source= [
            "bt_client.c",
            "bt_peermanager.c",
            "bt_piece.c",
            "bt_piece_db.c",
            "bt_diskmem.c",
            "bt_diskcache.c",
            "bt_filedumper.c",
            "bt_choker_seeder.c",
            "bt_choker_leecher.c",
            "bt_selector_random.c",
            "bt_selector_rarestfirst.c",
            "bt_selector_sequential.c",
            "bt_util.c",
            "bt_sha1.c",
            "bt_string.c",
            "readfile.c",
            "sha1.c",
            cp+"CBag/bag.c",
            cp+"CHeap/heap.c",
            cp+"CConfig-re/list.c",
            cp+"CConfig-re/config.c",
            cp+"CBitfield/bitfield.c",
            cp+"CMeanQueue/meanqueue.c",
            cp+"CPSeudoLRU/pseudolru.c",
            cp+"CEventTimer/event_timer.c",
            cp+"CSimpleBitstream/bitstream.c",
            cp+"CSparseCounter/sparse_counter.c",
            cp+"CHeaplessBencodeReader/bencode.c",
            cp+"CBTPWPConnection/pwp_connection.c",
            cp+"CBTPWPConnection/pwp_msghandler.c",
            cp+"CBTPWPConnection/pwp_handshaker.c",
            cp+"CLinkedListQueue/linked_list_queue.c",
            cp+"CTorrentFileReader/torrentfile_reader.c",
            cp+"CHashMapViaLinkedList/linked_list_hashmap.c",
            cp+"CSparseFileAllocator/sparsefile_allocator.c",
            ],
        #bt_diskmem.c
        #CCircularBuffer/cbuffer.c
        #use='config',
        target='yabbt',
        lib = libs,
        includes=[
            cp+"CHeaplessBencodeReader",
            cp+"CLinkedListQueue",
            cp+"CHashMapViaLinkedList",
            cp+"CHeap",
            cp+"CPSeudoLRU",
            cp+"CBipBuffer",
            cp+"CEventTimer",
            cp+"CSparseCounter",
            cp+"CSparseFileAllocator",
            cp+"CBitfield",
            cp+"CBTPWPConnection",
            cp+"CSimpleBitstream",
            cp+"CConfig-re",
            cp+"CBTTrackerClient",
            cp+"CBag",
            cp+"CMeanQueue",
           ], 
        cflags=[
            '-Werror',
            '-g',
            platform,
            '-Werror=uninitialized',
            '-Werror=pointer-to-int-cast',
            '-Werror=return-type',
            '-Wcast-align'],
        )

    unittest(bld,"test_bt.c")
    unittest(bld,"test_peermanager.c")
    unittest(bld,'test_choker_leecher.c')
    unittest(bld,'test_choker_seeder.c')
    unittest(bld,'test_rarestfirst.c')
    unittest(bld,'test_selector_random.c')
    unittest(bld,'test_selector_sequential.c')
    unittest(bld,'test_piece.c',ccflag='-I../'+cp+"CBitfield")
    unittest(bld,'test_piece_db.c')
    end2end(bld,'test_end_to_end.c')


    libs = ['yabbt','uv']
    if sys.platform == 'win32':
        libs += ['ws2_32']
        libs += ['psapi']
        libs += ['Iphlpapi']
    else:
        libs += ['dl']
        libs += ['rt']
        libs += ['pthread']

    bld.program(
        source=[
            "mt19937ar.c",
            'yabtorrent.c',
            "networkfuncs_libuv.c",
            cp+"CBipBuffer/bipbuffer.c",
            cp+"CBTTrackerClient/url_encoder.c",
            cp+"CBTTrackerClient/tracker_http.c",
            cp+"CBTTrackerClient/tracker_client.c",
            cp+"CBTTrackerClient/tracker_http_response_reader.c",
            cp+"CBTTrackerClient/http-parser/http_parser.c",
            ],
        target='yabtorrent',
        cflags=[
            '-g',
            '-Werror',
            '-Werror=uninitialized',
            '-Werror=pointer-to-int-cast',
            '-Werror=return-type'
            ],
        stlibpath = ['./libuv','.'],
        lib = libs,
        includes=[
            './libuv/include',
            cp+"CBipBuffer",
            cp+"CConfig-re",
            cp+"CLinkedListQueue",
            cp+"CBTTrackerClient",
            cp+"CTorrentFileReader",
            cp+"CHashMapViaLinkedList",
            cp+"CHeaplessBencodeReader",
            cp+"CBTTrackerClient/http-parser",
           ])

