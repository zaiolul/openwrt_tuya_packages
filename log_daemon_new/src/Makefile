
BIN:=log_daemon
SRCS:=$(wildcard *.c)
OBJS:=$(SRCS:.c=.o)

LIB:= -llink_core -lmiddleware_implementation -lplatform_port -lutils_modules -largp\
-lubus -lubox -lblobmsg_json
#LDFLAGS:=-L../sdk/build/lib $(LIB)
INC:= -I./includes
 #-I../sdk/utils -I../sdk/include -I../sdk/interface -I../sdk/libraries/coreJSON/source/include 
CC:=gcc
CFLAGS = -Wall -g

.PHONY: all clean

all:$(BIN)
	
$(BIN): $(SRCS)
	$(CC) -o $@ $^ $(INC) $(LIB)

clean:
	rm $(BIN) 