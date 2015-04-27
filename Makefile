GTEST_ROOT = ./3rd/gtest
GTEST_MAKEFILE = ./3rd/gtest/Makefile
TEST_ROOT = testcase
SRCS = $(wildcard *.c)
OBJS = $(SRCS:%.c=%.o)
TEST_SRCS = $(wildcard testcase/*.cpp)
TEST_OBJS = $(TEST_SRCS:%.cpp=%.o)
TEST_BINS = $(TEST_SRCS:%.cpp=%)
TEST_INCLUDE = . $(GTEST_ROOT)/include
TEST_LIBS = rt pthread
ifdef clang
CC = clang
CXX = clang++
AR = ar
COMM_CFLAGS = -Wall -g -Wfloat-equal -fno-omit-frame-pointer -fsanitize=address
else
CC = gcc
CXX = g++
AR = ar
COMM_CFLAGS = -Wall  -Wno-strict-aliasing -DLINUX64 -g -Wfloat-equal -rdynamic \
	-Wformat=2 -Wno-cast-qual -fno-omit-frame-pointer -fno-builtin-memcpy -Wbad-function-cast -Wextra -Wno-sign-compare -Wextra -Wbad-function-cast \
	-D_LARGEFILE_SOURCE -D_FILE_OFFSET_BITS=64
# -Wconversion -Wsign-compare
endif

CXXFLAGS = $(COMM_CFLAGS) -std=c++11

$(GTEST_ROOT)/configure:
	@cd ./3rd;rm -Rf gtest;unzip gtest-1.7.0.zip;mv gtest-1.7.0 gtest;
$(GTEST_ROOT)/Makefile:$(GTEST_ROOT)/configure
	@cd $(GTEST_ROOT);./configure --enable-static --disable-shared;
$(GTEST_ROOT)/src/gtest-all.o:$(GTEST_ROOT)/Makefile
	@cd $(GTEST_ROOT);make src/gtest-all.o

%.o:%.c
	$(CC) -c $(CFLAGS) $(addprefix -I,$(INCLUDE)) $^ -o $@
	
$(TEST_ROOT)/%.o:$(TEST_ROOT)/%.cpp
	$(CXX) $(CXXFLAGS) -c $(addprefix -I,$(TEST_INCLUDE)) $^ -o $@

$(TEST_ROOT)/%:$(OBJS) $(TEST_ROOT)/%.o $(GTEST_ROOT)/src/gtest-all.o
	$(CXX) $(CXXFLAGS) $^ $(addprefix -l,$(TEST_LIBS)) -o $@

test:$(TEST_BINS)

runtest:test
	rm -f $(TEST_ROOT)/*.o
	for CASE in $(TEST_BINS); do \
	$$CASE; \
	echo "";\
	done

runtestxml:test
	rm -f $(TEST_ROOT)/*.o
	for CASE in $(TEST_BINS); do \
		$$CASE --gtest_output="xml:$$CASE.xml"; \
	echo "";\
	done

clean:
	rm -f $(BASICOBJS)
	rm -f testcase/*.o
	rm -f $(TEST_BINS)
	rm -f $(OUTPUT)
	rm -f $(DUMPBIN)

cleanall:clean
	rm -rf ./3rd/expat
	rm -rf ./3rd/json-c
	rm -rf ./3rd/gtest
	rm -f $(TEST_BINS)
	rm -f $(TEST_ROOT)/*.o
	rm -f example/*.o
	@if [ -e "$(GTEST_MAKEFILE)" ]; then cd $(GTEST_ROOT);make clean;cd -; fi;
