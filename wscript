def options(opt):
        opt.load('compiler_c')

def configure(conf):
        conf.load('compiler_c')


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
                use='ebt',
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
#        bld.stlib(source='a.c', target='mystlib')
        bld.objects(
                source='config.c list.c',
                target='config',
                cflags=[
                    '-g',
                    '-fPIC',
                    '-Werror'
                ]
                )
#        bld.objects(
#                source='bencode/bencode.c',
#                target='bencode',
#                cflags=['-g', '-fPIC']
#                )

        bld.shlib(
            source='bencode/bencode.c bt_client.c bt_sha1.c bt_util.c bt_piece_db.c bt_filedumper.c bt_bitfield.c bt_piece.c byte_reader.c raprogress.c bt_metafile_reader.c bt_tracker_response_reader.c bt_choker_leecher.c bt_choker_seeder.c url_encoder.c http_request.c bt_peer_connection.c bt_diskcache.c bt_diskmem.c readfile.c sha1.c chashmap_via_linked_list/linked_list_hashmap.c clinkedlistqueue/linked_list_queue.c cheap/heap.c bt_rarestfirst_selector.c pseudolru/pseudolru.c bt_ticker.c',
                use='config',
                target='ebt',
                includes='bencode clinkedlistqueue chashmap_via_linked_list cheap pseudolru',
                cflags=[
                    '-Werror',
                    '-g',
                    '-Werror=uninitialized',
                    '-Werror=return-type',
                    '-Wcast-align'],
                )

#                use='config bencode',
#        bld(rule='../tests_bt', target='tests_bt.c')

        unittest(bld,'test_bt.c')
        unittest(bld,'test_bitfield.c')
        unittest(bld,'test_bt.c')
        unittest(bld,'test_byte_reader.c')
        unittest(bld,'test_filedumper.c')
#        unittest(bld,'test_metafile.c')
        unittest(bld,'test_piece.c')
        unittest(bld,'test_piece_db.c')
#        unittest(bld,'test_pwp_event_manager.c')
        unittest(bld,'test_raprogress.c')
        unittest(bld,'test_peer_connection.c')
        unittest(bld,'test_choker_leecher.c')
        unittest(bld,'test_choker_seeder.c')
        unittest(bld,'test_rarestfirst.c')
#        unittest(bld,'test_peer_connection_read.c')
#        unittest(bld,'test_peer_connection_send.c')

        bld.program(
                source='bt_main.c bt_main_network.c ccircularbuffer/cbuffer.c',
                target='bt',
                includes='ccircularbuffer',
                cflags=[
                    '-g',
                    '-Werror',
                    '-Werror=uninitialized',
                    '-Werror=return-type'
                    ],
                use='ebt')



