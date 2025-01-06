#include "process-simulator.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <stdbool.h>
#include <assert.h>
#include <stdint.h>

// function to handle the fork event
/**
 * @param current_process Pointer to the current process
 */
void run_fork(PCB **current_process)
{
    PCB *new_process = (PCB *)malloc(sizeof(PCB));
    assert(new_process != NULL);

    memcpy(new_process, *current_process, sizeof(PCB));

    new_process->pid = (*current_process)->pid + 1;

    new_process->parent = *current_process;
    new_process->next = NULL;

    PCB *last_process = *current_process;
    while (last_process->next != NULL)
    {
        last_process = last_process->next;
    }

    last_process->next = new_process;

    *current_process = new_process;
}

// function to handle the exec event
/**
 * @param program_name Name of the program to execute
 * @param vector_table Pointer to the vector table
 * @param file Pointer to the output file
 * @param external_files Pointer to the external files array
 * @param external_file_count Number of external files
 * @param memory_partitions Pointer to the memory partitions array
 * @param current_process Pointer to pointer of the current process
 * @param current_time Pointer to the current time
 * @param duration Duration of the event
 */
void run_exec(const char *program_name, const int *vector_table, FILE *file, ExternalFile *external_files, int external_file_count, MemoryPartition *memory_partitions, PCB **current_process, uint16_t *current_time, uint16_t duration)
{
    // Check if the current process is not the FIRST call to exec() (init process)
    bool is_init = (*current_process)->pid == 11;
    int program_size = -1;

    if (!(is_init))
    {
        // 1. Find the size of the program from the external files
        for (int i = 0; i < external_file_count; i++)
        {
            if (strcmp(external_files[i].program_name, program_name) == 0)
            {
                program_size = external_files[i].size;
                break;
            }
        }

        if (program_size == -1)
        {
            printf("Error: Program %s not found in external files\n", program_name);
            return;
        }
    }
    else
    {
        program_size = 1; // init process size is 1
    }

    // 2. Find the best fit memory partition for the program
    // best fit algorithm searches the entire memory partitions array for the best fit.
    MemoryPartition *candidate_partition = NULL;
    for (int i = 0; i < MAX_PARTITIONS; i++)
    {
        if (strcmp(memory_partitions[i].code, "free") == 0 && memory_partitions[i].size >= program_size)
        {
            if (candidate_partition == NULL || memory_partitions[i].size < candidate_partition->size)
            {
                candidate_partition = &memory_partitions[i];
            }
        }
    }
    // Check if a suitable partition was found
    if (candidate_partition == NULL)
    {
        printf("Error: No suitable partition found for program %s\n", program_name);
        return;
    }

    // Check if the current process is not the FIRST call to exec() (init process)
    // now that we have all the information we can start the fprintf process
    if ((!is_init))
    {

        // Random values for EXEC events THAT match the duration of the event.
        uint16_t a = rand() % (duration + 1);         // load program into memory
        uint16_t b = rand() % (duration - a + 1);     // find partition
        uint16_t c = rand() % (duration - a - b + 1); // mark partition as occupied
        uint16_t d = duration - a - b - c;            // update PCB with new information

        fprintf(file, "%hu, %d, EXEC: load %s of size %dMb\n", *current_time, a, program_name, program_size);
        *current_time += a;
        fprintf(file, "%hu, %d, found partition %hu with %huMb of space\n", *current_time, b, candidate_partition->partition_number, program_size);
        *current_time += b;
        fprintf(file, "%hu, %d, partition %hu marked as occupied\n", *current_time, c, candidate_partition->partition_number);
        *current_time += c;
        fprintf(file, "%hu, %d, updating PCB with new information\n", *current_time, d);
        *current_time += d;
        fprintf(file, "%hu, 1, scheduler called\n", *current_time);
        *current_time += 1;
        fprintf(file, "%hu, 1, IRET\n", *current_time);
    }

    // 3. Mark the partition as occupied with the program name
    strcpy(candidate_partition->code, program_name);

    // 4. Update the PCB with the new information
    (*current_process)->partition_number = candidate_partition->partition_number;
    strcpy((*current_process)->program_name, (is_init) ? "init" : program_name);
    (*current_process)->program_size = program_size;

    // find the head of the pcb table
    PCB *pcb_table = *current_process;
    while (pcb_table->parent != NULL)
    {
        pcb_table = pcb_table->parent;
    }

    if (!is_init)
    {
        save_system_status(*current_time, pcb_table);
        *current_time += 1;
    }
    // 5. Load the trace file for the program
    TraceEvent trace_events[MAX_EVENTS];
    int event_count = 0;

    load_trace(program_name, trace_events, &event_count);

    // 6. Call process_trace to run the process
    process_trace(trace_events, event_count, vector_table, file, external_files, external_file_count, memory_partitions, *current_process, current_time);

    // once execution is done, return execution to parent
    *current_process = (*current_process)->parent;
}

// Function to handle the system status
/**
 * @param current_time Current time
 * @param pcb_table Pointer to the PCB table
 */
void save_system_status(uint16_t current_time, PCB *pcb_table)
{
    // this was a problem with the file needing to be opened multiple times and not to be cleared but rather to be cleared on the first run (once per simulation)

    static int first_run = 1;
    FILE *status_file;

    if (first_run)
    {
        status_file = fopen("logs/system_status.txt", "w");
        first_run = 0;
    }
    else
    {
        status_file = fopen("logs/system_status.txt", "a");
    }

    if (!status_file)
    {
        printf("Error: Cannot open system_status.txt for writing\n");
        return;
    }

    fprintf(status_file, "!----------------------------------------------------------!\n");
    fprintf(status_file, "Save Time: %hu ms\n", current_time);
    fprintf(status_file, "+-----------------------------------------------+\n");
    fprintf(status_file, "| PID  | Program Name | Partition Number | Size |\n");
    fprintf(status_file, "+-----------------------------------------------+\n");

    PCB *current = pcb_table;
    current = current->next;
    while (current != NULL)
    {
        fprintf(status_file, "| %-4hu | %-12s | %-16hu | %-4hu |\n", current->pid, current->program_name, current->partition_number, current->program_size);
        current = current->next;
    }

    fprintf(status_file, "+-----------------------------------------------+\n");
    fprintf(status_file, "!----------------------------------------------------------!\n");

    fclose(status_file);
}

// Declaration of the load_external_files function
/**
 * @param filename Name of the file to load
 * @param external_files Pointer to the external files array
 * @param external_file_count Pointer to the external file count
 */
void load_external_files(const char *filename, ExternalFile *external_files, int *external_file_count)
{
    FILE *file = fopen(filename, "r"); // Open file for reading
    if (!file)
    {
        printf("Error: Cannot open file %s\n", filename);
        exit(1);
    }

    // buffer to store each line
    char line[256];

    // read each line, parse it, store in the external_files array
    while (fgets(line, sizeof(line), file))
    {
        ExternalFile current_file;

        // Parse
        if (sscanf(line, "%[^,], %hu", current_file.program_name, &current_file.size) == 2)
        {
            // Store the parsed external file in the external_files array
            external_files[*external_file_count] = current_file;
            (*external_file_count)++;
        }
        else
        {
            printf("Error: Line format not recognized: %s", line);
        }
    }

    fclose(file);
}

// Function to load trace events from a file
/**
 * @param filename Name of the file to load
 * @param trace Pointer to the trace array
 * @param event_count Pointer to the event count
 */
void load_trace(const char *filename, TraceEvent *trace, int *event_count)
{
    // check if filename has a .txt extension (trap)
    bool trap = (strstr(filename, ".txt") == (filename + strlen(filename) - 4));

    // create a new filename with .txt extension
    char new_filename[20];
    strcpy(new_filename, filename);
    strcat(new_filename, ".txt");

    // open the file for reading
    FILE *file = (trap) ? fopen(filename, "r") : fopen(new_filename, "r");

    // -----------------------------------------------------------
    // Open the trace file
    // -----------------------------------------------------------
    if (strncmp(filename, "program", 7) == 0)
    {
        // Construct the new filename with the additional path
        char new_filename[256];
        snprintf(new_filename, sizeof(new_filename), "additionalFiles/%s.txt", filename);
        file = fopen(new_filename, "r");
    }
    else
    {
        file = fopen(filename, "r");
    }

    // Check if the file was successfully opened
    if (!file)
    {
        printf("Error: Cannot open file %s\n", (trap) ? filename : new_filename);
        exit(1);
    }

    // buffer to store each line
    char line[256];

    // read each line, parse it, store in the trace array
    while (fgets(line, sizeof(line), file))
    {
        TraceEvent current_event;
        current_event.vector = -1; // Default vector to -1

        // Check if the line contains "CPU" OR "END_IO" OR "SYSCALL" OR "FORK" OR "EXEC"
        if (sscanf(line, "CPU, %hu", &current_event.duration) == 1)
        {
            strcpy(current_event.type, "CPU");
        }
        else if (sscanf(line, "END_IO %hd, %hu", &current_event.vector, &current_event.duration) == 2)
        {
            strcpy(current_event.type, "END_IO");
        }
        else if (sscanf(line, "SYSCALL %hd, %hu", &current_event.vector, &current_event.duration) == 2)
        {
            strcpy(current_event.type, "SYSCALL");
        }
        else if (sscanf(line, "FORK, %hu", &current_event.duration) == 1)
        {
            strcpy(current_event.type, "FORK");
            current_event.vector = 2; // default vector is 2
        }
        else if (sscanf(line, "EXEC %[^,], %hu", current_event.program_name, &current_event.duration) == 2)
        {
            strcpy(current_event.type, "EXEC");
            current_event.vector = 3; // default vector is 3
        }
        else
        {
            printf("Error: Line format not recognized: %s", line);
            continue; // Skip to the end of the loop
        }

        // Store the parsed event in the trace array
        trace[*event_count] = current_event;
        (*event_count)++;
    }

    fclose(file);
}

// Function to load the vector table from a file
/**
 * @param filename Name of the file to load
 * @param vector_table Pointer to the vector table array
 */
void load_vector_table(const char *filename, int *vector_table)
{
    FILE *file = fopen(filename, "r");
    if (!file)
    {
        printf("Error: Cannot open file %s\n", filename);
        exit(1);
    }

    char line[256];
    int index = 0;

    // Read each line from the file and parse it into vector table array
    while (fgets(line, sizeof(line), file))
    {
        int is_success = (sscanf(line, "%X", &vector_table[index]) == 1);
        index += (is_success) ? 1 : 0;

        if (!is_success)
        {
            printf("Error: Line format not recognized: %s", line); // Debug print for unrecognized format
        }
    }
    fclose(file);
}

// Function to process the trace and log the events
/**
 * @param trace Pointer to the trace array
 * @param event_count Number of events in the trace
 * @param vector_table Pointer to the vector table array
 * @param file Pointer to the output file
 * @param external_files Pointer to the external files array
 * @param external_file_count Number of external files
 * @param partitions Pointer to the memory partitions array
 * @param current_process Pointer to the current process
 * @param current_time Pointer to the current time
 */
void process_trace(TraceEvent *trace, int event_count, const int *vector_table, FILE *file, ExternalFile *external_files, int external_file_count, MemoryPartition *partitions, PCB *current_process, uint16_t *current_time)
{
    // check if the file is NULL
    if (!file)
    {
        printf("Error: Cannot open output file\n");
        exit(1);
    }

    // Used to alternate between the two SYSCALL events (display and transfer data)
    bool which_syscall = false;
    // check if were in init (used for ASSUMPTION METHOD 2, can be ignored.)
    bool is_init = (current_process->pid == (11 - 1));
    // iterate through the current process's parent to get the pcb table
    PCB *pcb_table = current_process;
    while (pcb_table->parent != NULL)
    {
        pcb_table = pcb_table->parent;
    }

    // Loop through each event in the trace
    for (int i = 0; i < event_count; i++)
        if (strcmp(trace[i].type, "CPU") == 0) // Check if the event is a CPU event
        {
            fprintf(file, "%d, %d, CPU execution\n", *current_time, trace[i].duration);
            *current_time += trace[i].duration;
        }
        else if (strcmp(trace[i].type, "SYSCALL") == 0) // Check if the event is a SYSCALL event
        {
            // Random values for the SYSCALL events THAT match the duration of the event.
            int duration = trace[i].duration;
            int a = rand() % (duration + 1);     // for run the ISR
            int b = rand() % (duration - a + 1); // for transfer data
            int c = duration - a - b;            // check for errors

            fprintf(file, "%d, 1, switch to kernel mode\n", *current_time);
            *current_time += 1;
            int context_time = (rand() % 3) + 1; // random context switch time (1-3)
            fprintf(file, "%d, %d, context saved\n", *current_time, context_time);
            *current_time += context_time;
            fprintf(file, "%d, 1, find vector %d in memory position 0x%04X\n", *current_time, trace[i].vector, trace[i].vector * 2);
            *current_time += 1;
            fprintf(file, "%d, 1, load address 0X%04X into the PC\n", *current_time, vector_table[trace[i].vector]);
            *current_time += 1;
            fprintf(file, "%d, %d, SYSCALL: run the ISR\n", *current_time, a);
            *current_time += a;
            if (which_syscall)
            {
                fprintf(file, "%d, %d, transfer data to display\n", *current_time, b);
            }
            else
            {
                fprintf(file, "%d, %d, transfer data\n", *current_time, b);
            }
            which_syscall = !which_syscall;

            *current_time += b;
            fprintf(file, "%d, %d, check for errors\n", *current_time, c);
            *current_time += c;
            fprintf(file, "%d, 1, IRET\n", *current_time);
            save_system_status(*current_time, pcb_table);
            *current_time += 1;
        }
        else if (strcmp(trace[i].type, "END_IO") == 0) // Check if the event is an END_IO event
        {
            fprintf(file, "%d, 1, check priority of interrupt\n", *current_time);
            *current_time += 1;
            fprintf(file, "%d, 1, check if masked\n", *current_time);
            *current_time += 1;
            fprintf(file, "%d, 1, switch to kernel mode\n", *current_time);
            *current_time += 1;
            fprintf(file, "%d, 3, context saved\n", *current_time);
            *current_time += 3;
            fprintf(file, "%d, 1, find vector %d in memory position 0x%04X\n", *current_time, trace[i].vector, trace[i].vector * 2);
            *current_time += 1;
            fprintf(file, "%d, 1, load address 0X%04X into the PC\n", *current_time, vector_table[trace[i].vector]);
            *current_time += 1;
            fprintf(file, "%d, %d, END_IO\n", *current_time, trace[i].duration);
            *current_time += trace[i].duration;
            fprintf(file, "%d, 1, IRET\n", *current_time);
            save_system_status(*current_time, pcb_table);
            *current_time += 1;
        }
        else if (strcmp(trace[i].type, "FORK") == 0) // Check if the event is a FORK event
        {
            if (!(is_init))
            {
                // "Random" values for FORK events THAT match the duration of the event.
                int duration = trace[i].duration;
                int a = rand() % (duration + 1); // for copy parent PCB to child PCB
                int b = duration - a;            // for scheduler called

                fprintf(file, "%d, 1, switch to kernel mode\n", *current_time);
                *current_time += 1;
                fprintf(file, "%d, 3, context saved\n", *current_time);
                *current_time += 3;
                fprintf(file, "%d, 1, find vector %d in memory position 0x%04X\n", *current_time, trace[i].vector, trace[i].vector * 2);
                *current_time += 1;
                fprintf(file, "%d, 1, load address 0X%04X into the PC\n", *current_time, vector_table[trace[i].vector]);
                *current_time += 1;
                fprintf(file, "%d, %d, FORK: copy parent PCB to child PCB\n", *current_time, a);
                *current_time += a;
                fprintf(file, "%d, %d, scheduler called\n", *current_time, b);
                *current_time += b;
                fprintf(file, "%d, 1, IRET\n", *current_time);
            }
            run_fork(&current_process);
            save_system_status(*current_time, pcb_table);
            *current_time += 1; // insure we take a snapshot of PCB table before we increment time
        }
        else if (strcmp(trace[i].type, "EXEC") == 0) // Check if the event is an EXEC event
        {
            if (!(is_init))
            {
                fprintf(file, "%hu, 1, switch to kernel mode\n", *current_time);
                *current_time += 1;
                uint8_t context_time = (rand() % 3) + 1; // random context switch time (1-3)
                fprintf(file, "%hu, %u, context saved\n", *current_time, context_time);
                *current_time += context_time;
                fprintf(file, "%hu, 1, find vector %d in memory position 0x%04X\n", *current_time, trace[i].vector, trace[i].vector * 2);
                *current_time += 1;
                fprintf(file, "%hu, 1, load address 0X%04X into the PC\n", *current_time, vector_table[trace[i].vector]);
                *current_time += 1;
            }
            run_exec(trace[i].program_name, vector_table, file, external_files, external_file_count, partitions, &current_process, current_time, trace[i].duration);
        }
}

// Function to initialize a PCB
/**
 * @param pcb Pointer to the PCB to initialize
 * @return Pointer to the initialized PCB
 */
PCB *init_pcb(PCB *pcb)
{
    pcb->pid = 10; // because we will fork and the init process will have a pid of 11, even tho i think init should start at 0...
    pcb->cpu_time = 0;
    pcb->io_time = 0;
    pcb->remaining_cpu_time = 0;
    pcb->partition_number = 6;
    strcpy(pcb->program_name, "init");
    pcb->program_size = 1;
    pcb->parent = NULL;
    pcb->next = NULL;
    return pcb;
}

// Function to free the PCB linked list
/**
 * @param pcb_table Pointer to the PCB linked list
 */
void free_pcb_list(PCB *pcb_table)
{
    PCB *temp;
    PCB *current = pcb_table;
    while (current != NULL)
    {
        temp = current;
        current = current->next;
        free(temp);
    }
}

// Main function to handle command-line arguments and call the appropriate functions
int main(int argc, char *argv[])
{
    if (argc != 5)
    {
        printf("Usage: %s <trace_file> <external_files> <vector_table_file> <output_file>\n", argv[0]);
        return 1;
    }

    // Seed
    srand(time(NULL));

    // -----------------------------------------------------------
    // Loading
    // -----------------------------------------------------------

    // Load trace events, ASSUMPION: init is initialized by a trace file
    // TraceEvent trace_events[MAX_EVENTS];
    // int event_count = 0;
    // load_trace(argv[1], trace_events, &event_count);

    // Load external files
    ExternalFile external_files[MAX_EXTERNAL_FILES];
    int external_file_count = 0;
    load_external_files(argv[2], external_files, &external_file_count);

    // Load vector table
    int vector_table[VECTOR_TABLE_SIZE];
    load_vector_table(argv[3], vector_table);

    // -----------------------------------------------------------
    // Initialization
    // -----------------------------------------------------------

    // Initialize memory partitions
    MemoryPartition partitions[MAX_PARTITIONS] = {{1, 40, "free"}, {2, 25, "free"}, {3, 15, "free"}, {4, 10, "free"}, {5, 8, "free"}, {6, 2, "free"}};

    // Open the output file to stream the output
    FILE *file = fopen(argv[4], "w");
    if (!file)
    {
        printf("Error: Cannot open output file %s\n", argv[4]);
        return 1;
    }

    // Initialize PCB (linked list) with the init template
    PCB pcb_head;
    PCB *current_process = init_pcb(&pcb_head);

    // Initialize the current time
    uint16_t current_time = 0;

    // -----------------------------------------------------------
    // Simulation
    // -----------------------------------------------------------

    // ASSUMPION: init is initialized by a trace file
    // process_trace(trace_events, event_count, vector_table, file, external_files, external_file_count, partitions, current_process, &current_time);

    // Fork the init process
    run_fork(&current_process);
    // snapshot the pcb table
    save_system_status(current_time, &pcb_head);
    // run the simulation
    run_exec(argv[1], vector_table, file, external_files, external_file_count, partitions, &current_process, &current_time, 0);

    // -----------------------------------------------------------
    // Cleanup
    // -----------------------------------------------------------

    printf("Simulation complete\n");
    fclose(file);                 // Close the output file
    free_pcb_list(pcb_head.next); // free the PCB linked list

    // -----------------------------------------------------------
    // Debugging Section
    // -----------------------------------------------------------

    if (DEBUG_MODE)
    {
        // Print the output trace
        char choice;
        printf("Execution trace saved to %s\n", argv[3]);
        printf("Would you like to print the execution trace? (y/n): ");
        scanf("%c", &choice);

        if ((choice == 'y') || (choice == 'Y'))
        {
            FILE *file = fopen(argv[3], "r"); // Open file for reading
            if (!file)
            {
                printf("Error: Cannot open file %s\n", argv[3]); // Print error if file can't be opened
            }

            int i = 0;
            char line[256];
            // Read each line from the file and print it
            while (fgets(line, sizeof(line), file))
            {
                printf("Execution Trace Line %d: ", i);
                printf("%s", line);
                i++;
            }
        }

        printf("\n\tGoodbye %s!\n\n", argv[1]);
    }
    return 0;
}
