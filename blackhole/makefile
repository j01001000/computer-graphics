# Black hole ray tracer
EXE=blackhole

# Main target
all: $(EXE)

# Platform-specific compiler and linker options
ifeq ($(OS),Windows_NT)
   CFLG=-O3 -Wall -fopenmp -DUSEGLEW
   LIBS=-lOpenCL -lglfw3 -lglew32 -lglu32 -lopengl32 -lm
   CLEAN=rm -f *.exe *.o *.a
else ifeq ($(shell uname -s),Darwin)
   BREW_PREFIX := $(shell brew --prefix)
   CFLG=-O3 -Wall -Wno-deprecated-declarations -DUSEGLEW -I$(BREW_PREFIX)/include
   LIBS=-L$(BREW_PREFIX)/lib -lglfw -lGLEW \
        -framework Cocoa -framework OpenGL \
        -framework IOKit -framework OpenCL
   CLEAN=rm -f $(EXE) *.o *.a
else
   CFLG=-O3 -Wall -fopenmp
   LIBS=-lOpenCL -lglfw -lGLU -lGL -lm
   CLEAN=rm -f $(EXE) *.o *.a
endif

# Dependencies
CSCIx239.h: mat4.h
InitGPUcl.o: InitGPUcl.cpp InitGPUcl.h
$(EXE).o: $(EXE).cpp CSCIx239.h InitGPUcl.h RayMarch.h
RayMarch.o: RayMarch.cpp RayMarch.h
fatal.o: fatal.c CSCIx239.h
errcheck.o: errcheck.c CSCIx239.h
print.o: print.c CSCIx239.h
axes.o: axes.c CSCIx239.h
loadtexbmp.o: loadtexbmp.c CSCIx239.h
loadobj.o: loadobj.c CSCIx239.h
projection.o: projection.c CSCIx239.h
lighting.o: lighting.c CSCIx239.h
elapsed.o: elapsed.c CSCIx239.h
fps.o: fps.c CSCIx239.h
shader.o: shader.c CSCIx239.h
noise.o: noise.c CSCIx239.h
cube.o: cube.c CSCIx239.h
sphere.o: sphere.c CSCIx239.h
cone.o: cone.c CSCIx239.h
cylinder.o: cylinder.c CSCIx239.h
torus.o: torus.c CSCIx239.h
icosahedron.o: icosahedron.c CSCIx239.h
teapot.o: teapot.c CSCIx239.h
mat4.o: mat4.c CSCIx239.h
initwin.o: initwin.c CSCIx239.h

#  Create archive
CSCIx239.a: fatal.o errcheck.o print.o axes.o loadtexbmp.o loadobj.o projection.o lighting.o elapsed.o fps.o shader.o noise.o cube.o sphere.o cone.o cylinder.o torus.o icosahedron.o teapot.o mat4.o initwin.o
	ar -rcs $@ $^

# Compile rules
%.o: %.c
	gcc -c $(CFLG) $< -o $@

%.o: %.cpp
	g++ -c $(CFLG) $< -o $@

#  Link
$(EXE): $(EXE).o RayMarch.o InitGPUcl.o CSCIx239.a
	g++ $(CFLG) -o $@ $^ $(LIBS)

#  Clean
clean:
	$(CLEAN)

.PHONY: all clean
