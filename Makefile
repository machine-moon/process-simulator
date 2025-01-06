# Compiler
CC = gcc

# Compiler flags
CFLAGS = -Wall -g

# Source files
SRC = src/process-simulator.c
OBJ = process-simulator.o

# Header files
DEPS = src/process-simulator.h

# Executable names
TARGET = sim

# Default rule (build everything)
all: $(TARGET) rm-obj

# Remove object files
rm-obj:
	rm -f $(OBJ) 


# Object file rule
$(OBJ): $(SRC) $(DEPS)
	$(CC) $(CFLAGS) -c src/process-simulator.c

# Build the executable from object file 
$(TARGET): $(OBJ)
	$(CC) $(CFLAGS) -o $(TARGET) $(OBJ)

# Clean up
clean:
	rm -f $(TARGET) $(OBJ) logs/*.txt 



################## TESTS ##################

# Test 1: Run the program with different arguments
test1: $(TARGET)
	./$(TARGET) tests/trace_1.txt additionalFiles/external_files.txt additionalFiles/vector_table.txt logs/execution1.txt

# Test 2: Run the program with different arguments 
test2: $(TARGET)
	./$(TARGET) tests/trace_2.txt additionalFiles/external_files.txt additionalFiles/vector_table.txt logs/execution2.txt

# Test 3: Run the program with different arguments 
test3: $(TARGET)
	./$(TARGET) tests/trace_3.txt additionalFiles/external_files.txt additionalFiles/vector_table.txt logs/execution3.txt

# Test 4: Run the program with different arguments 
test4: $(TARGET)
	./$(TARGET) tests/trace_4.txt additionalFiles/external_files.txt additionalFiles/vector_table.txt logs/execution4.txt

# Test 5: Run the program with different arguments 
test5: $(TARGET)
	./$(TARGET) tests/trace_5.txt additionalFiles/external_files.txt additionalFiles/vector_table.txt logs/execution5.txt

test: test1 test2 test3 test4 test5 rm-obj