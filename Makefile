# xdd: xendevd

verbose	?= n
debug	?= n


APP	:=
APP	+= $(patsubst %.c, %, $(shell find app/ -name "*.c"))

LIB	:=
LIB	+= $(patsubst %.c, %.o, $(shell find lib/xdd/ -name "*.c"))

INC	:=
INC	+= $(shell find inc/ -name "*.h")

LIB_XDDCONN_SERVER_OBJ	:= lib/xddconn/server.o
LIB_XDDCONN_SERVER_SHARED	:= lib/libxddconn-server.so
LIB_XDDCONN_SERVER_STATIC	:= lib/libxddconn-server.a

LIB_XDDCONN_CLIENT_OBJ	:= lib/xddconn/client.o
LIB_XDDCONN_CLIENT_SHARED	:= lib/libxddconn-client.so
LIB_XDDCONN_CLIENT_STATIC	:= lib/libxddconn-client.a

LIB_XDDCONN :=
LIB_XDDCONN += $(LIB_XDDCONN_SERVER_SHARED) $(LIB_XDDCONN_SERVER_STATIC)
LIB_XDDCONN += $(LIB_XDDCONN_CLIENT_SHARED) $(LIB_XDDCONN_CLIENT_STATIC)


CFLAGS		+= -Iinc -Wall -g -O3
LDFLAGS		+= -lxenstore

ifeq ($(debug),y)
CFLAGS		+= -DDEBUG
endif


include make.mk

all: $(LIB_XDDCONN) $(APP)

app/xendevd: LDFLAGS += -ludev -Llib -lxddconn-server
$(APP): % : %.o $(LIB)
	$(call clink, $^, $@)

$(LIB_XDDCONN_SERVER_SHARED): CFLAGS += -fPIC
$(LIB_XDDCONN_SERVER_SHARED): $(LIB_XDDCONN_SERVER_OBJ)
	$(call cmd, "LIB", $@, gcc -shared $^ -o $@, )

$(LIB_XDDCONN_CLIENT_SHARED): CFLAGS += -fPIC
$(LIB_XDDCONN_CLIENT_SHARED): $(LIB_XDDCONN_CLIENT_OBJ)
	$(call cmd, "LIB", $@, gcc -shared $^ -o $@, )

$(LIB_XDDCONN_SERVER_STATIC): $(LIB_XDDCONN_SERVER_OBJ)
	$(call cmd, "LIB", $@, ar rcs $@, $^)

$(LIB_XDDCONN_CLIENT_STATIC): $(LIB_XDDCONN_CLIENT_OBJ)
	$(call cmd, "LIB", $@, ar rcs $@, $^)


%.o: %.c $(INC)
	$(call ccompile, $<, $@)


clean:
	$(call cmd, "CLN", "*.o [ app/  ]", rm -rf, $(patsubst %, %.o, $(APP)))
	$(call cmd, "CLN", "*.o [ lib/  ]", rm -rf, $(LIB) $(LIB_XDDCONN_SERVER_OBJ) $(LIB_XDDCONN_CLIENT_OBJ))

distclean: clean
	$(call cmd, "CLN", "* [ app/  ]" , rm -rf, $(APP))
	$(call cmd, "CLN", "* [ lib/  ]" , rm -rf, $(LIB_XDDCONN))


.PHONY: all clean distclean
