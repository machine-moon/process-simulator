# Interrupt Simulator
## Description
A simple Interrupt Simulator for SYSC-4001 A2.

## Usage

To run the simulator, use the following command:
```sh
./sim <trace_file> <external_files> <vector_table_file> <output_file>
```

## Makefile Instructions

### Default Rule
To build the program use:
```sh
make
```

### Running Specific Tests
To run specific tests, use the following commands:
```sh
make test1
make test2
make test3
make test4
make test5
```
Note: system_status.txt will get overwritten after each test


### Running All Tests
To run all tests, use:
```sh
make test_all
```

### Cleaning Up
To remove the executable, object file, and ALL test execution files in, use:
```sh
make clean
```

## Assumptions and Design Decisions

### Initial Approach
Initially, I created a template struct for the init process that would fork itself and start simulating traces. However, Brightspace instructions indicated the trace.txt file should include the init process.

### Challenge with init in trace.txt
Including init in trace.txt raised questions:
- How would init self-replicate without creating a loop?
- Including init in a separate trace file would be ideal but would require an init_trace.txt with only a fork and an exec call, contradicting the required submission list.

### Chosen Design
My simulator starts with empty memory; exec assigns init to the correct partition and labels it. Tests specify that init should already be running when `./sim` starts, implying it should be forked and executed by the simulator. The ambiguity on whether the simulator or trace.txt should initialize init is trivial in implementation but impacts clarity and design consistency.

### Submission and Testing
The Brightspace submission list had typos and updates post-reading week. I aligned the design with the testing framework and the instructions as understood. The difference:
- **Simulator (OS-Handled)**: Calls fork, followed by exec, invoking the simulator.
- **Trace.txt (User-Handled)**: Calls the simulator directly, which then forks and execs init.

### Alternate Method
To shift initialization responsibility to trace.txt, comment out the fork, save_system_status, and exec lines, uncomment process_trace, along with loading trace, and recompile with `make`. This approach would require an extra file not specified on Brightspace.

### External Programs
Based on examples, I am assuming sizes relative to:
  - **COMMAND:    time:size**
  - **cpu_size**:   100:10
  - **syscall**:    125:15
  - **fork**:       (same as syscall)
  - **exec**:       (same as syscall)
  - **end_IO**:     >= cpu ratio