#Page#7 in pdf
#0.2 Makefile
#Write a simple Makefile that generates an executable sshell from the file sshell.c, using
#GCC.
#The compiler should be run with the -Wall -Wextra (enable all warnings, and
#some more) and -Werror (treat all warnings as errors) flags.
#There should also be a clean rule that removes any generated files and puts the
#directory back in its original state.


CC = gcc
CFLAGS = -Wall -Wextra -Werror
TARGET = sshell
SRC = sshell.c

all: $(TARGET)

$(TARGET): $(SRC)
	$(CC) $(CFLAGS) -o $(TARGET) $(SRC)

clean:
	rm -f $(TARGET)
