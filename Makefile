CC=gcc
CFLAGS=-g -O3 -MMD -lm -lOpenCL
PGR?=ising
OBJS=oclsim

EXECFILE=$(addprefix build/, $(PGR))
OBJFILE=$(addsuffix .o, $(addprefix ./build/, $(OBJS)))
OBJSRC=$(addsuffix .c, $(addprefix ./, $(OBJS)))
PGRSRC=$(addsuffix .c, $(addprefix ./, $(PGR)))

all: build/ $(EXECFILE)

build/:
	mkdir -p build/

$(EXECFILE): $(PGRSRC) $(OBJFILE)
	$(CC) $(CFLAGS) $(PGRSRC) -o $(EXECFILE) $(OBJFILE)

$(OBJFILE): $(OBJSRC)
	$(CC) $(CFLAGS) -c $(OBJSRC) -o $(OBJFILE)

-include $(wildcard build/*.d)

.PHONY: clean tar zip

clean:
	rm -r build/

tar: $(EXECFILE) Makefile
	tar --create --file $(PGR).tar.gz --gzip Makefile \
	$$(find -maxdepth 2 -regextype awk -regex ".*\.(c|h|hex|md)" -printf "%P ")

zip: build/$(PGR).hex Makefile
	7z a $(PGR).zip Makefile \
	$$(find -maxdepth 2 -regextype awk -regex ".*\.(c|h|hex|md)" -printf "%P ")
