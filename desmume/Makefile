PRGNAME     = desmume.elf

# define regarding OS, which compiler to use
CC = gcc
LD  = gcc

# change compilation / linking flag options
CC_OPTS		= -O1 -g3 -Wall -Wextra
CFLAGS		= $(CC_OPTS)

CFLAGS 		+= -DFASTBUILD -DHOST_LINUX=1 -I.  -I./src -D_GNU_SOURCE=1 -D_REENTRANT -DINCLUDE_OPENGL_2D
CFLAGS      += -Isrc -Isrc/cli -Isrc/addons -Isrc/utils -Isrc/utils/tinyxml -Isrc/utils/libfat -Isrc/utils/decrypt
LDFLAGS     = -lSDL -lm -lasound -lstdc++ -pthread -lz -lGL -lGLU

# Files to be compiled
SRCDIR    = ./src ./src/cli
VPATH     = $(SRCDIR)
SRC_C   = $(foreach dir, $(SRCDIR), $(wildcard $(dir)/*.c))
OBJ_C   = $(notdir $(patsubst %.c, %.o, $(SRC_C)))
OBJS     = $(OBJ_C)

all: $(PRGNAME)

# Rules to make executable
$(PRGNAME): $(OBJS)  
	$(CC) $(CFLAGS) -o $(PRGNAME) $^ $(LDFLAGS)

$(OBJ_C) : %.o : %.c
	$(CC) $(CFLAGS) -c -o $@ $<

clean:
	rm -f *.o
