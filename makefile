
DIR_INC=./include
DIR_SRC=./src
DIR_BIN=.
DIR_OBJ=./obj
DIR_LIB=./lib

#sources_all := $(shell find . -name "*.c" -o -name "*.cpp" -o -name "*.h")
#sources_c   := $(filter %.c, $(sources_all))
#sources_h   := $(filter %.h, $(sources_all))

sources_c   := $(wildcard ./src/*.c)
sources_h   := $(wildcard ./include/*.h)

INC_HEADER := $(sort $(dir $(sources_h)))
DIR_INC := $(strip $(subst / ./,:./,$(INC_HEADER)))
DIR_INC += #add a NULL on trail
DIR_INC := $(strip $(subst / , ,$(DIR_INC)))

VPATH=$(DIR_INC)
VPATH_INC=$(patsubst %,-I%,$(subst :, ,${VPATH}))

SRC=$(sources_c)

CLRDIR=$(notdir ${SRC})
OBJ=$(patsubst %.c,${DIR_OBJ}/%.o,$(CLRDIR))

CC =arm-linux-gnueabihf-gcc
LD =arm-linux-gnueabihf-gcc

LDFLAGS = -v -lgcc -lstdc++ -lm -lc -lgcc_s 

LDFLAGS += -L${DIR_LIB} -Wl,-rpath,`pwd`/${DIR_LIB}  -lssl -lcrypto

TARGET=xiaoaimqtt

BIN_TARGET=${DIR_BIN}/${TARGET}


.PHONY : clean rebuild all

all: $(BIN_TARGET) 

%.d:%.c 
	set -e; rm -f $@; \ $(CC) -MM $(VPATH_INC) $< > $@.$$$$; \ 
	sed 's,\($*\)\.o[ :]*,\1.o $@ : ,g' < $@.$$$$ > $@; \ 
	rm -f $@.$$$$
    
include $(SOURCES:c=.d)
    
$(BIN_TARGET):$(OBJ)
	$(LD) $(LDFLAGS) -o $@ $^ -lpthread -lm
    

${DIR_OBJ}/%.o:${DIR_SRC}/%.c
	$(CC) $(VPATH_INC) -c $< -o $@ 



clean:
	rm -rf $(DIR_OBJ)/*.o $(BIN_TARGET)

rebuild: clean all
