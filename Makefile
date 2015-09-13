LIBUV_BRANCH=v1.2.1

lets_build_this:
	python waf configure
	python waf build
.PHONY : default_target

clean:
	python waf clean || true
	cd deps/libuv && make clean > /dev/null
.PHONY : clean

libuv_build:
	cd deps/libuv && sh autogen.sh
	cd deps/libuv && ./configure
	cd deps/libuv && make
	cp deps/libuv/.libs/libuv.a .
.PHONY : libuv_build

libuv_fetch:
	if test -e deps/libuv; \
	then cd deps/libuv && git pull origin $(LIBUV_BRANCH); \
	else git clone https://github.com/libuv/libuv deps/libuv; \
	fi
	cd deps/libuv && git checkout $(LIBUV_BRANCH)
.PHONY : libuv_fetch

libuv: libuv_fetch libuv_build
.PHONY : libuv

libuv_vendor:
	rm -rf deps/libuv/.git > /dev/null
.PHONY : libuv_vendor

vendordeps: libuv_vendor
.PHONY : vendordeps

usage.c:
	docopt2ragel USAGE > src/usage.rl
	ragel src/usage.rl
