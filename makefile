# Compiler and flags
CC= gcc -pedantic -ansi -Wall -g
PROFILE= profile.txt

# Top-level build target
all: life

# Build the life application
life: life.o life_util.o
	${CC} -o life life.o life_util.o -lpthread

# Build the compare application
compare: life_util.o
	${CC} -o compare compare.c life_util.o

life.o: life.c
	${CC} -c life.c

life_util.o: life_util.c
	${CC} -c life_util.c

# Run tests to ensure correct behavior
test: life compare
	./life -n 1024 -r 1 -t 1 -i input/input_1024.txt -o temp.txt
	./compare 1024 input/output_1024.txt temp.txt
	rm temp.txt
	./life -n 1024 -r 1 -t 2 -i input/input_1024.txt -o temp.txt
	./compare 1024 input/output_1024.txt temp.txt
	rm temp.txt
	./life -n 1024 -r 1 -t 3 -i input/input_1024.txt -o temp.txt
	./compare 1024 input/output_1024.txt temp.txt
	rm temp.txt
	./life -n 1024 -r 1 -t 4 -i input/input_1024.txt -o temp.txt
	./compare 1024 input/output_1024.txt temp.txt
	rm temp.txt
	./life -n 1024 -r 1 -t 5 -i input/input_1024.txt -o temp.txt
	./compare 1024 input/output_1024.txt temp.txt
	rm temp.txt
	./life -n 1024 -r 1 -t 6 -i input/input_1024.txt -o temp.txt
	./compare 1024 input/output_1024.txt temp.txt
	rm temp.txt
	./life -n 1024 -r 1 -t 7 -i input/input_1024.txt -o temp.txt
	./compare 1024 input/output_1024.txt temp.txt
	rm temp.txt
	./life -n 1024 -r 1 -t 8 -i input/input_1024.txt -o temp.txt
	./compare 1024 input/output_1024.txt temp.txt
	rm temp.txt
	./life -n 1024 -r 1 -t 9 -i input/input_1024.txt -o temp.txt
	./compare 1024 input/output_1024.txt temp.txt
	rm temp.txt
	./life -n 1024 -r 1 -t 10 -i input/input_1024.txt -o temp.txt
	./compare 1024 input/output_1024.txt temp.txt
	rm temp.txt
	rm compare

# Profiles the program with different settings
profile: life compare
	echo ./life -n 1024 -r 1 -t 1 -i input/input_1024.txt -o temp.txt > ${PROFILE}
	time ./life -n 1024 -r 1 -t 1 -i input/input_1024.txt -o temp.txt >> profile.txt
	rm temp.txt
	rm compare

# Removes built files
clean:
	rm life
	rm life.o
	rm life_util.o
