RM=rm -rf
CC=gcc

C_SRCS=$(shell find . -iname "*.c" | tr '\n' ' ')
H_SRCS=$(shell find . -iname "*.h" | tr '\n' ' ')
OBJS=$(C_SRCS:./%.c=build/%.o)

all: build/libcompartidas.so

create-dirs:
	mkdir -p build/funcionesCompartidas

build/libcompartidas.so: create-dirs $(OBJS)
	$(CC)  -shared -Wall -Wextra -o "build/libcompartidas.so" $(OBJS)

build/funcionesCompartidas/%.o: funcionesCompartidas/%.c
	$(CC)  -c -Wall -Wextra -fmessage-length=0 -fPIC -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -o "$@" "$<"

clean:
	$(RM) build

install: all
	cp -u build/libcompartidas.so /usr/lib
	cp --parents -u $(H_SRCS) /usr/include
	rm -r build/

uninstall:
	rm -f /usr/lib/libcompartidas.so
	rm -rf /usr/include/funcionesCompartidas

.PHONY: all create-dirs clean install uninstall
