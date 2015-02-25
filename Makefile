# xdd: xendevd

verbose	?= n
debug	?= n


APP	:=
APP	+= $(patsubst %.c, %, $(shell find app/ -name "*.c"))

LIB	:=
LIB	+= $(patsubst %.c, %.o, $(shell find lib/ -name "*.c"))

INC	:=
INC	+= $(shell find inc/ -name "*.h")


CFLAGS		+= -Iinc -Wall -g -O3
LDFLAGS		+= -lxenstore

ifeq ($(debug),y)
CFLAGS		+= -DDEBUG
endif


include make.mk

all: $(APP)

$(APP): % : %.o $(LIB)
	$(call clink, $^, $@)

%.o: %.c $(INC)
	$(call ccompile, $<, $@)


clean:
	$(call cmd, "CLN", "*.o [ app/  ]", rm -rf, $(patsubst %, %.o, $(APP)))
	$(call cmd, "CLN", "*.o [ lib/  ]", rm -rf, $(LIB))

distclean: clean
	$(call cmd, "CLN", "* [ app/  ]" , rm -rf, $(APP))


.PHONY: all clean distclean
