#compiler shizzle

CC=avr-gcc
CFLAGS=-mmcu=atmega2561 -Os -Wall -Wstrict-prototypes -DNUT_CPU_FREQ=14745600 \
			-D__HARVARD_ARCH__ -DNUTOS_VERSION=433 \
			-Wa,-ahlms=$(SRC_DIR)/$*lst
ASFLAGS=-mmcu=atmega2561 -I. -x assembler-with-cpp -Wa,-ahlms=$(SRC_DIR)/$*lst,-gstabs 
LDFLAGS=-mmcu=atmega2561 -Wl,--defsym=main=0,-Map=TIStreamer.map,--cref
LD=ld

# lib shit

# Application source en include includes
SRC_DIR=../source
INC_DIR=../include

# NutOS location (includes and libs)
NUT_INC=c:/ethernut-4.3.3/nut/include
NUT_BUILD_INC=c:/ethernut-4.3.3/build/gcc/atmega2561/lib/include
NUT_LIB_DIR=c:/ethernut-4.3.3/build/gcc/atmega2561/lib

# WinAvr includes
AVR_INC=c:/winavr/avr/include

#INC total
INC=-$(NUT_LIB_DIR)/nutinit.o -lnutpro -lnutnet -lnutpro -lnutfs -lnutos -lnutdev -lnutarch -lnutnet -lnutcrt -lnutdev


# targets
NET=net.o
HTTP=http.o
INET=inet.o

$(NET): $(HTTP) $(INET)
	
$(HTTP): http.c
	$(CC) -c $(CFLAGS) -I$(INC_DIR) -I$(NUT_INC) -I$(AVR_INC) -o http.o
	
$(INET): inet.c
	$(CC) -c $(CFLAGS) -I$(INC_DIR) -I$(NUT_INC) -I$(AVR_INC) -o http.o
