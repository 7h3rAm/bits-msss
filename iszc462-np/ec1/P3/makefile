CC=gcc
CFLAGS=-Wall -O3 -c -ggdb -fstack-protector-all -Wstack-protector -D_FORIFY_SOURCE=2 -z noexecstack -fpie -pie
LDFALGS=-Wl,-z,relro,-z,now
SOURCES=P3.c
OBJECTS=$(SOURCES:.c=.o)
EXECUTABLE=P3

all: $(SOURCES) $(EXECUTABLE)

$(EXECUTABLE): $(OBJECTS)
	$(CC) $(LDFLAGS) $(OBJECTS) -o $@

.c.o:
	$(CC) $(CFLAGS) $< -o $@

clean:
	rm -rf $(OBJECTS) $(EXECUTABLE)

