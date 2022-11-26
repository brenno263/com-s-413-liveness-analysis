CC=gcc
COMPILER_FLAGS=-O1 -Wall -std=c99
LINKER_FLAGS=-lm
EXE=livealyzer

ODIR=obj


# These are the object files we want to compile.
# Remember to add new project files to this list.
OBJ=main.o
# Convert all OBJs from "asdf.o" to "$(ODIR)/asdf.o".
OBJ:=$(patsubst %,$(ODIR)/%,$(OBJ))

all: $(EXE)

# All .o files depend on .c files.
$(ODIR)/%.o: %.c
	$(CC) -c $< -o $@ $(COMPILER_FLAGS)

# Executable depends on all .o files.
$(EXE): $(OBJ)
	$(CC) $^ -o $@ $(LINKER_FLAGS)

# These are meant to be used as commands.
# PHONY prevents associating with any similarly named files.
.PHONY: clean run

# Remove transient files.
clean:
	rm -f $(EXE) $(ODIR)/*.o *~ gmon.out *.data

# Compile & run.
run: $(EXE)
	./$(EXE)
