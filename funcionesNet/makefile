RM=rm -rf
CC=gcc

C_SRCS=$(shell find . -iname "*.c" | tr '\n' ' ')
H_SRCS=$(shell find . -iname "*.h" | tr '\n' ' ')
OBJS=$(C_SRCS:./%.c=build/%.o)

# nombre de la biblio compartida
all: build/libnetfuncs.so

create-dirs:
	mkdir -p build/funcionesNet

build/libnetfuncs.so: create-dirs $(OBJS)
	$(CC)  -shared -Wall -Wextra -o "build/libnetfuncs.so" $(OBJS)

build/funcionesNet/%.o: funcionesNet/%.c
	$(CC)  -c -Wall -Wextra -fmessage-length=0 -fPIC -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -o "$@" "$<"

# Clean build files
clean:
	$(RM) build

install: all
	cp -u build/libnetfuncs.so /usr/lib
	cp --parents -u $(H_SRCS) /usr/include
	clean

uninstall:
	rm -f /usr/lib/libnetfuncs.so
	rm -rf /usr/include/funcionesNet

.PHONY: all create-dirs clean install uninstall
