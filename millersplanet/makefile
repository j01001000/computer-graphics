# Project
EXEEXT =

# Platform-specific compiler and linker options
ifeq ($(OS),Windows_NT)
EXEEXT = .exe
CPPFLAGS += -DUSEGLEW
LDLIBS += -lfreeglut -lglew32 -lglu32 -lopengl32 -lm
else ifeq ($(shell uname -s),Darwin)
CFLAGS += -Wno-deprecated-declarations
LDLIBS += -framework GLUT -framework OpenGL
else
LDLIBS += -lglut -lGLU -lGL -lm
endif

EXE = project$(EXEEXT)
LIB = CSCIx229.a
LIB_OBJS = fatal.o errcheck.o print.o loadtexbmp.o loadobj.o projection.o
OBJS = project.o $(LIB_OBJS)

CC = gcc
AR = ar
CFLAGS += -O3 -Wall
ARFLAGS = rcs

.PHONY: all clean

# Main target
all: $(EXE)

# Dependencies
$(OBJS): CSCIx229.h

# Create archive
$(LIB): $(LIB_OBJS)
	$(AR) $(ARFLAGS) $@ $^

# Compile C sources
%.o: %.c
	$(CC) $(CPPFLAGS) $(CFLAGS) -c $< -o $@

# Link executable
$(EXE): project.o $(LIB)
	$(CC) $(LDFLAGS) -o $@ $^ $(LDLIBS)

# Remove generated build artifacts for the current platform
clean:
	$(RM) $(EXE) $(OBJS) $(LIB)
