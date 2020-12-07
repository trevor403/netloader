OBJECTS	= build/crt0.o build/exception.o build/cache.o build/main.o build/memory.o build/exceptionc.o build/exi.o build/video.o build/graphics.o build/pad.o build/message.o build/debugmenus.o build/text.o build/bba.o build/cs-net.o build/cs-main.o

AS	= powerpc-eabi-as
CC	= powerpc-eabi-gcc
LD	= powerpc-eabi-ld
STRIP	= powerpc-eabi-strip
OBJDUMP	= powerpc-eabi-objdump
OBJCOPY = powerpc-eabi-objcopy


LFLAGS	= -Ttext 0x81300000
LIBFLAGS = 
CFLAGS	= -gdwarf-2 -O0 -DGEKKO -mogc -mcpu=750 -meabi -mhard-float -ftree-loop-distribute-patterns

NAME	= netloader

all: ${NAME}.dol

clean:
	rm -f ${OBJECTS} ${NAME}.dol ${NAME}.elf ${NAME}.bin

${NAME}.dol: ${NAME}.elf
	$(OBJDUMP) -S -D ${NAME}.elf > disasm.txt
	${STRIP} --strip-unneeded ${NAME}.elf
	elf2dol -v ${NAME}.elf ${NAME}.dol
	rm ${NAME}.elf

${NAME}.elf: ${OBJECTS}
	${LD} ${LFLAGS} -o $@ $^ ${LIBFLAGS}

build/crt0.o: crt0.s
	${AS} -o $@ crt0.s

build/exception.o: exception.S
	${AS} -o $@ exception.S

build/cache.o: cache.s
	${AS} -o $@ cache.s

build/main.o: main.c
	${CC} -c ${CFLAGS} -o $@ main.c

build/memory.o: memory.c
	${CC} -c ${CFLAGS} -o $@ memory.c

build/exceptionc.o: exceptionc.c
	${CC} -c ${CFLAGS} -o $@ exceptionc.c

build/exi.o: exi.c
	${CC} -c ${CFLAGS} -o $@ exi.c

build/video.o: video.c
	${CC} -c ${CFLAGS} -o $@ video.c

build/graphics.o: graphics.c
	${CC} -c ${CFLAGS} -o $@ graphics.c

build/pad.o: pad.c
	${CC} -c ${CFLAGS} -o $@ pad.c

build/message.o: message.c
	${CC} -c ${CFLAGS} -o $@ message.c

build/debugmenus.o: debugmenus.c
	${CC} -c ${CFLAGS} -o $@ debugmenus.c

build/text.o: text.c
	${CC} -c ${CFLAGS} -o $@ text.c

build/bba.o: bba.c
	${CC} -c ${CFLAGS} -o $@ bba.c

build/cs-net.o: cs-net.c
	${CC} -c ${CFLAGS} -o $@ cs-net.c

build/cs-main.o: cs-main.c
	${CC} -c ${CFLAGS} -o $@ cs-main.c
