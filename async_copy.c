#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <errno.h>
#include <unistd.h>
#include <string.h>
#include <aio.h>
#include <time.h>




// Define the maximum number of I/O operations that can be pending at the same time.
#define MAX_IO_OPERATIONS 64

// Structure to hold the control block information for our asynchronous I/O operations.
struct aiocb aiocb_list[MAX_IO_OPERATIONS];




// -------------------------------------------------------------------------------------------------------
// Function to set up an asynchronous read operation.
// -------------------------------------------------------------------------------------------------------
void aio_read_setup(struct aiocb *aiocbp, int fd, off_t offset, volatile void *buf, size_t size)
{
    memset(aiocbp, 0, sizeof(struct aiocb)); // Clear out the aiocb structure to zero.
    aiocbp->aio_fildes = fd;                 // File descriptor for the file to read from.
    aiocbp->aio_buf = buf;                   // Buffer to read the data into.
    aiocbp->aio_nbytes = size;               // Number of bytes to read.
    aiocbp->aio_offset = offset;             // Offset in the file to start reading from.
    if (aio_read(aiocbp) == -1)              // Initiate the read operation.
    {
        perror("aio_read");
        exit(EXIT_FAILURE);
    }
}




// -------------------------------------------------------------------------------------------------------
// Function to set up an asynchronous write operation.
// -------------------------------------------------------------------------------------------------------
void aio_write_setup(struct aiocb *aiocbp, int fd, off_t offset, volatile void *buf, size_t size)
{
    memset(aiocbp, 0, sizeof(struct aiocb)); // Clear out the aiocb structure to zero.
    aiocbp->aio_fildes = fd;                 // File descriptor for the file to write to.
    aiocbp->aio_buf = buf;                   // Buffer with the data to write.
    aiocbp->aio_nbytes = size;               // Number of bytes to write.
    aiocbp->aio_offset = offset;             // Offset in the file to start writing to.
    if (aio_write(aiocbp) == -1)             // Initiate the write operation.
    {
        perror("aio_write");
        exit(EXIT_FAILURE);
    }
}




// -------------------------------------------------------------------------------------------------------
// Function to wait for all asynchronous I/O operations to complete.
// -------------------------------------------------------------------------------------------------------
void wait_for_aio_operations(struct aiocb *aiocbp_list, int num_ops)
{
    for (int i = 0; i < num_ops; i++)
    {
        while (aio_error(&aiocbp_list[i]) == EINPROGRESS)
        {
            usleep(1000); // Busy waiting is generally bad practice, but used here for simplicity.
        }
        int ret = aio_return(&aiocbp_list[i]); // Retrieve the return status of the I/O operation.
        if (ret == -1)
        {
            perror("aio_return");
            exit(EXIT_FAILURE);
        }
    }
}




// -------------------------------------------------------------------------------------------------------
// MAIN Function
// -------------------------------------------------------------------------------------------------------
int main()
{
    char source_path[256];
    char destination_path[256];
    size_t block_size;
    int num_async_ops;
    int source_fd, destination_fd;
    off_t offset = 0;
    ssize_t bytes_read, bytes_written;
    struct timespec start, finish;
    double elapsed;

    // Title
    // -------------------------------------------------------------------------------------------------------
    printf("\033[0;33;40m"); // Yellow console text color
    puts("====================================================");
    puts(" Linux Async Copying ");
    puts("====================================================");
    printf("\033[0m"); // Normal text color

    // Prompt the user for the source and destination file paths, block size, and number of async operations.
    // -------------------------------------------------------------------------------------------------------
    
    printf("\033[0;34;40m"); // Blue text color
    printf("Enter the source file path: ");
    printf("\033[0m"); // Normal text color
    scanf("%255s", source_path);

    printf("\033[0;34;40m"); // Blue text color
    printf("Enter the destination file path: ");
    printf("\033[0m"); // Normal text color
    scanf("%255s", destination_path);

    printf("\033[0;34;40m"); // Blue text color
    printf("Enter the block size in KB for copying: ");
    printf("\033[0m"); // Normal text color
    scanf("%zu", &block_size);
    block_size *= 1024; // Convert the block size from KB to bytes.

    printf("\033[0;34;40m"); // Blue text color
    printf("Enter the number of asynchronous operations: ");
    printf("\033[0m"); // Normal text color
    scanf("%d", &num_async_ops);

    // Open the source file for reading and the destination file for writing (creating it if it doesn't exist)
    // -------------------------------------------------------------------------------------------------------
    source_fd = open(source_path, O_RDONLY);
    if (source_fd == -1) 
    {
        perror("open source");
        exit(EXIT_FAILURE);
    }
    destination_fd = open(destination_path, O_WRONLY | O_CREAT, 0644);
    if (destination_fd == -1) 
    {
        perror("open destination");
        exit(EXIT_FAILURE);
    }

    // Allocate a buffer to hold the data for asynchronous operations.
    // -------------------------------------------------------------------------------------------------------
    char *buffer = (char *)malloc(block_size * num_async_ops);
    if (!buffer) 
    {
        perror("malloc");
        exit(EXIT_FAILURE);
    }

    // Start timing the copy operation.
    clock_gettime(CLOCK_MONOTONIC, &start);

    // Set up and initiate the first batch of read operations.
    for (int i = 0; i < num_async_ops; i++) 
    {
        aio_read_setup(&aiocb_list[i], source_fd, offset, buffer + (i * block_size), block_size);
        offset += block_size; // Increment the file offset for the next operation.
    }

    // Loop to handle the asynchronous read and write operations.
    // -------------------------------------------------------------------------------------------------------
    int ops_in_progress = num_async_ops; // Keep track of the number of operations in progress.
    while (ops_in_progress > 0) 
    {
        wait_for_aio_operations(aiocb_list, num_async_ops); // Wait for the read operations to complete.
        for (int i = 0; i < num_async_ops; i++) {
            bytes_read = aio_return(&aiocb_list[i]); // Check how many bytes were read.
            if (bytes_read > 0) 
            {
                // Set up and initiate the write operation corresponding to the read operation that just completed.
                aio_write_setup(&aiocb_list[i], destination_fd, aiocb_list[i].aio_offset, (void *)aiocb_list[i].aio_buf, bytes_read);
            } 
            else 
            {
                // If read returned 0, we're at the end of the file, so decrement the operations count.
                ops_in_progress--;
            }
        }
        wait_for_aio_operations(aiocb_list, num_async_ops); // Wait for the write operations to complete.
        for (int i = 0; i < num_async_ops; i++) {
            bytes_written = aio_return(&aiocb_list[i]); // Check how many bytes were written.
            if (bytes_written > 0) 
            {
                // If there's more data to read, set up the next read operation.
                aio_read_setup(&aiocb_list[i], source_fd, offset, buffer + (i * block_size), block_size);
                offset += block_size; // Increment the file offset for the next read operation.
            }
        }
    }

    // Stop timing the copy operation.
    clock_gettime(CLOCK_MONOTONIC, &finish);

    // Calculate the total time taken for the copy operation.
    elapsed = (finish.tv_sec - start.tv_sec);
    elapsed += (finish.tv_nsec - start.tv_nsec) / 1000000000.0;

    // Clean up: close the file descriptors and free the allocated buffer.
    close(source_fd);
    close(destination_fd);
    free(buffer);

    // Green text
    printf("\033[0;32;40m");
    // Print the result of the copy operation.
    printf("\nCopying was completed in %f seconds\n", elapsed);
    // Normal text color
    printf("\033[0m");

    return 0;
}
