##########################################################
##########################################################
CC=g++
CFLAGS=-O3 -c

SRC=libs/stm/src
TESTS=libs/stm/test
HEADERS1=.
HEADERS2=.
HEADERS3=.

LDFLAGS=-lpthread
INCLUDES=-I $(SRC) -I $(TESTS) -I $(HEADERS1) -I $(HEADERS2) -I $(HEADERS3) -I ./../../$(SRC) -I ./../../$(TESTS) -I ./../../$(HEADERS1) -I ./../../$(HEADERS2) -I ./../../$(HEADERS3) -I ./../$(SRC) -I ./../$(TESTS) -I ./../$(HEADERS1) -I ./../$(HEADERS2) -I ./../$(HEADERS3) -I ./../../../$(SRC) -I ./../../../$(TESTS) -I ./../../../$(HEADERS1) -I ./../../../$(HEADERS2) -I ./../../../$(HEADERS3) -I ./../../../../$(SRC) -I ./../../../../$(TESTS) -I ./../../../../$(HEADERS1) -I ./../../../../$(HEADERS2) -I ./../../../../$(HEADERS3)


SOURCES=$(SRC)/contention_manager.cpp $(SRC)/transaction.cpp $(SRC)/bloom_filter.cpp $(TESTS)/globalIntArr.cpp $(TESTS)/irrevocableInt.cpp $(TESTS)/isolatedComposedIntLockInTx2.cpp $(TESTS)/isolatedComposedIntLockInTx.cpp $(TESTS)/isolatedInt.cpp $(TESTS)/isolatedIntLockInTx.cpp $(TESTS)/litExample.cpp $(TESTS)/lotExample.cpp $(TESTS)/nestedTxs.cpp $(TESTS)/smart.cpp $(TESTS)/stm.cpp $(TESTS)/testHashMap.cpp $(TESTS)/testHashMapAndLinkedListsWithLocks.cpp $(TESTS)/testHashMapWithLocks.cpp $(TESTS)/testHT_latm.cpp $(TESTS)/testInt.cpp $(TESTS)/testLinkedList.cpp $(TESTS)/test1writerNreader.cpp $(TESTS)/testLinkedListWithLocks.cpp $(TESTS)/testLL_latm.cpp $(TESTS)/testPerson.cpp $(TESTS)/testRBTree.cpp $(TESTS)/testRBTreeV2.cpp $(TESTS)/transferFun.cpp $(TESTS)/txLinearLock.cpp $(TESTS)/usingLockTx.cpp $(TESTS)/testatom.cpp $(TESTS)/pointer_test.cpp $(TESTS)/testEmbedded.cpp $(TESTS)/testBufferedDelete.cpp

OBJECTS=$(SOURCES:.cpp=.o)
EXECUTABLE=TBoost.STM

all: $(SOURCES) $(EXECUTABLE)

$(EXECUTABLE): $(OBJECTS)
	$(CC) $(OBJECTS) -o $@ $(LDFLAGS)

.cpp.o:
	$(CC) $(CFLAGS) $< -o $@ $(INCLUDES)

clean:
	rm -rf libs/stm/src/*.o libs/stm/test/*.o TBoost.STM


