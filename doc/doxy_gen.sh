#!/bin/sh

doxygen doxy_gen.cfg
moxygen ./xml -o tcp_client_library.md
rm -r xml