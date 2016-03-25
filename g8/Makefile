INC_DIR	= ./include
SRC_DIR = ./src
OBJ_DIR	= ./object
C_FILES = $(wildcard src/*_lib.c)
OBJ_FILES = $(patsubst src/%.c, object/%.o, $(C_FILES))

BINS = abt gbn sr

LIBS = 
CC	= gcc
CFLAGS	= -g -I$(INC_DIR)

all: $(BINS)

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c
	$(CC) -c -o $@ $< $(CFLAGS)

$(BINS): %: $(OBJ_DIR)/simulator.o $(OBJ_DIR)/%.o $(OBJ_FILES)
	$(CC) -o $@ $^ $(CFLAGS) $(LIBS)

clean:
	rm -f $(OBJ_DIR)/*.o $(INC_DIR)/*~ $(BINS)
