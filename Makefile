CFLAGS=-O3
FILES=src/main.c
OUT=build/gameplayer
INSTALL_DIR=/usr/bin

all: $(FILES)
	$(CC) $(FILES) $(CFLAGS) -o $(OUT)

install: $(OUT)
	cp $(OUT) $(INSTALL_DIR)
