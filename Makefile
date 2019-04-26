LOGLEVEL?=2
CC=cc
CFLAGS= -Wall -Wextra -Wfatal-errors -g3 -O0 -DLOGLEVEL=$(LOGLEVEL)
LIBS=-lwiringPi
PREFIX?=/usr/local

TARGET=libwsepd.a
OBJ=wsepd.o wsepd_signal.o

TEST_TGT=wsepd_test
TEST_OBJ=wsepd_test.o

.PHONY: all test clean install tags

all: $(TARGET)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@ $(LIBS)

%.a: $(OBJ)
	ar -rcs $@ $^

#test: LOGLEVEL=3
test: $(TEST_TGT)
$(TEST_TGT): $(TEST_OBJ) $(TARGET)
	$(CC) $(CFLAGS) $^ -o $@ $(LIBS)

clean:
	rm -f $(TARGET) $(OBJ) $(TEST_TGT) $(TEST_OBJ)

install: LOGLEVEL=1

tags:
	etags *.c *.h
