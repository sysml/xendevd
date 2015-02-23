# xdd: xendevd

verbose	?= n
debug	?= n


APP	:=
APP	+= $(patsubst %.c, %, $(shell find app/ -name "*.c"))

TST	:=
TST	+= $(patsubst %.c, %, $(shell find test/ -name "*.c"))

LIB	:=
LIB	+= $(patsubst %.c, %.o, $(shell find lib/ -name "*.c"))

INC	:=
INC	+= $(shell find inc/ -name "*.h")


CFLAGS		+= -Iinc -Wall -g -O3

ifeq ($(debug),y)
CFLAGS		+= -DDEBUG
endif


include make.mk

all: $(APP) $(TST)

$(APP): % : %.o $(LIB)
	$(call clink, $^, $@)

$(TST): % : %.o $(LIB)
	$(call clink, $^, $@)

%.o: %.c $(INC)
	$(call ccompile, $<, $@)


clean:
	$(call cmd, "CLN", "*.o [ app/  ]", rm -rf, $(patsubst %, %.o, $(APP)))
	$(call cmd, "CLN", "*.o [ test/ ]", rm -rf, $(patsubst %, %.o, $(TST)))
	$(call cmd, "CLN", "*.o [ lib/  ]", rm -rf, $(LIB))

distclean: clean
	$(call cmd, "CLN", "* [ app/  ]" , rm -rf, $(APP))
	$(call cmd, "CLN", "* [ test/ ]", rm -rf, $(TST))


.PHONY: all clean distclean
