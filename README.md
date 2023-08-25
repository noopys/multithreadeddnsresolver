# Multithreaded DNS Resolver


At the heart of this Multithreaded DNS Resolver lies a synchronization mechanism that employs semaphores to solve the classic producer/consumer problem. The system is designed to operate in a multi-threaded environment, where multiple requester threads act as producers and multiple resolver threads act as consumers. The threads interact with a shared, thread-safe array data structure, which serves as the buffer for DNS hostnames.

The synchronization mechanism is built on a triad of semaphores:

1. **Mutex Semaphore**: Ensures mutual exclusion when threads access the shared array, thereby preventing race conditions.
2. **Space Available Semaphore**: Blocks requester threads when the array is full, ensuring that the array never overflows.
3. **Items Available Semaphore**: Blocks resolver threads when the array is empty, ensuring that the threads only consume when there is data to process.

This intricate interplay of semaphores allows for a highly efficient, deadlock-free operation, ensuring that the system can scale seamlessly with increasing numbers of threads and data.

## Overview

The Multithreaded DNS Resolver is an OS-level DNS resolver written in C. It is designed to efficiently resolve a large number of domain names by utilizing multiple threads for both requesting and resolving DNS queries.

## Repository Structure

- `Makefile`: Contains the build instructions for the project.
- `README.md`: Brief description of the project.
- `array.c` and `array.h`: Implements the thread-safe array data structure.
- `multi-lookup.c` and `multi-lookup.h`: Contains the main logic for the DNS resolver.
- `util.c` and `util.h`: Utility functions for DNS lookup.

## Building the Project

To build the project, navigate to the project directory and run:

```bash
make
```

This will produce an executable named `multi-lookup`.

## Usage

Run the `multi-lookup` executable with the following command-line arguments:

```bash
./multi-lookup <numRequest> <numResolve> <servOutput> <resultOutput> <inputFiles...>
```

- `numRequest`: Number of requester threads.
- `numResolve`: Number of resolver threads.
- `servOutput`: Output file for serviced log.
- `resultOutput`: Output file for resolved DNS.
- `inputFiles`: One or more input files containing hostnames to resolve.

## Components

### Array Data Structure (`array.c` and `array.h`)

- `array_init(array *s)`: Initializes the array.
- `array_put(array *s, char *hostname)`: Inserts an element into the array.
- `array_get(array *s, char **hostname)`: Removes an element from the array.
- `array_free(array *s)`: Frees the array's resources.

### Main Logic (`multi-lookup.c` and `multi-lookup.h`)

- `requester(void *s)`: Requester thread function.
- `resolver(void *s)`: Resolver thread function.
- `writeToServiced(FILE *filename, char *line)`: Writes to the serviced log file.
- `writeToResolved(FILE *filename, char *hostname, char *ip, sem_t *writeResolved)`: Writes to the resolved DNS file.

### Utility Functions (`util.c` and `util.h`)

- `dnslookup(const char* hostname, char* firstIPstr, int maxSize)`: Performs DNS lookup.

## Limitations

- The number of requester threads should not exceed `MAX_REQUESTER_THREADS`.
- The number of resolver threads should not exceed `MAX_RESOLVER_THREADS`.
- The number of input files should not exceed `MAX_INPUT_FILES`.

## Additional Information

For more details, you can refer to the comments in the code files. Feel free to explore the intricacies of the advanced synchronization mechanisms and the multi-threaded architecture that make this DNS Resolver highly efficient and scalable.
