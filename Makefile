CC = gcc

DEADCODESTRIP = -Wl,-static \
								-fdata-sections \
								-ffunction-sections \
								-Wl,--gc-sections \
								-Wl,--strip-all

# -Wl,-static
# Link against static libraries.  Required for dead-code elimination

# -fdata-sections
# -ffunction-sections
# Keep data/functions in separate sections, so they can be discarded if unused

# Wl,--gc-sections
# tell the linker to garbage collect sections

# -s
# strip debug information
								
CFLAGS = -O3 -m64 -Wall -std=c99 -pedantic



# windows
LFLAGS = -L./lib 
INCLUDES = 
EXT = .exe

default: src/assembler.c
	$(CC) $(DEADCODESTRIP) $(CFLAGS) $(INCLUDES) \
  src/assembler.c  \
  -o dda$(EXT) $(LFLAGS) $(WIN_LIBS)
