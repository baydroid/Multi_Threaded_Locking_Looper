#   Copyright (c) 2020 transmission.aquitaine@yahoo.com
#
#   Licensed under the Apache License, Version 2.0 (the "License");
#   you may not use this file except in compliance with the License.
#   You may obtain a copy of the License at
#
#       http:#www.apache.org/licenses/LICENSE-2.0
#
#   Unless required by applicable law or agreed to in writing, software
#   distributed under the License is distributed on an "AS IS" BASIS,
#   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
#   See the License for the specific language governing permissions and
#   limitations under the License.

#GPP_OPTS  = -fPIC -O3 -D_REENTRANT -c -Wall -Werror -Wwrite-strings
#LINK_OPTS = -fPIC
GPP_OPTS  = -g -fPIC -D_REENTRANT -c -Wall -Werror -Wwrite-strings
LINK_OPTS = -g -fPIC

all : ../bin/MTLL_example

clean :
	rm -vf addr_width.h
	rm -vf ../o/*
	rm -vf ../bin/*

../o/make_addr_width.o : make_addr_width.cpp
	g++ $(GPP_OPTS) $< -o $@

../bin/make_addr_width : ../o/make_addr_width.o
	g++ $(LINK_OPTS) -o $@ ../o/make_addr_width.o

addr_width.h : ../bin/make_addr_width
	../bin/make_addr_width

basic_types.h : addr_width.h
	touch $@

MTLL.hpp : UintXTrieSet.hpp basic_types.h
	touch $@

../o/MTLL.o : MTLL.cpp MTLL.hpp
	g++ $(GPP_OPTS) $< -o $@

../o/MTLL_example.o : MTLL_example.cpp MTLL.hpp
	g++ $(GPP_OPTS) $< -o $@

../bin/MTLL_example : ../o/MTLL_example.o ../o/MTLL.o
	g++ $(LINK_OPTS) -lpthread -o $@ ../o/MTLL_example.o ../o/MTLL.o
