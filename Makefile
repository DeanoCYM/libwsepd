LOGLEVEL?=2
SPILOG?=0

CC=cc
CFLAGS= -Wall -Wextra -Wfatal-errors -g3 -O0 \
	-DLOGLEVEL=$(LOGLEVEL) -DSPILOG=$(SPILOG)

LIBS=-lwiringPi -lm
PREFIX?=/usr/local

TARGET=libwsepd.a
OBJ=wsepd.o wsepd_signal.o waveshare2.9.o wsepd_path.o

TEST_TGT=wsepd_test
TEST_OBJ=wsepd_test.o

.PHONY: all test clean install tags

all: $(TARGET)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@ $(LIBS)

%.a: $(OBJ)
	ar -rcs $@ $^

test: LOGLEVEL=3
test: $(TEST_TGT)
$(TEST_TGT): $(TEST_OBJ) $(TARGET)
	$(CC) $(CFLAGS) $^ -o $@ $(LIBS)

clean:
	rm -f $(TARGET) $(OBJ) $(TEST_TGT) $(TEST_OBJ)

install: LOGLEVEL=1

tags:
	etags *.c *.h
