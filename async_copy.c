#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <errno.h>
#include <unistd.h>
#include <string.h>
#include <aio.h>
#include <time.h>
#include <sys/stat.h>

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
    struct stat source_stat; // for getting file size
    off_t total_size; // total size of the source file
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
    // Get the size of the source file
    if (fstat(source_fd, &source_stat) == -1) 
    {
        perror("fstat");
        close(source_fd);
        exit(EXIT_FAILURE);
    }
    total_size = source_stat.st_size;
    destination_fd = open(destination_path, O_WRONLY | O_CREAT, 0644);
    if (destination_fd == -1) 
    {
        perror("open destination");
        close(source_fd);
        exit(EXIT_FAILURE);
    }

    // Allocate a buffer to hold the data for asynchronous operations.
    // -------------------------------------------------------------------------------------------------------
    char *buffer = (char *)malloc(block_size * num_async_ops);
    if (!buffer) 
    {
        perror("malloc");
        close(source_fd);
        close(destination_fd);
        exit(EXIT_FAILURE);
    }

    clock_gettime(CLOCK_MONOTONIC, &start);
    // Start the initial read operations
    for (int i = 0; i < num_async_ops; i++) 
    {
        aio_read_setup(&aiocb_list[i], source_fd, offset, buffer + (i * block_size), block_size);
        offset += block_size;
    }

    int ops_in_progress = num_async_ops;
    while (ops_in_progress > 0) 
    {
        for (int i = 0; i < num_async_ops; i++) 
        {
            int ret = aio_error(&aiocb_list[i]);
            if (ret == EINPROGRESS) continue;

            if (aiocb_list[i].aio_lio_opcode == LIO_READ) 
            {
                bytes_read = aio_return(&aiocb_list[i]);
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
            else if (aiocb_list[i].aio_lio_opcode == LIO_WRITE) 
            {
                bytes_written = aio_return(&aiocb_list[i]);
                if (bytes_written > 0 && offset < total_size) 
                {
                    // If there's more data to read, set up the next read operation.
                    aio_read_setup(&aiocb_list[i], source_fd, offset, buffer + (i * block_size), block_size);
                    offset += block_size;
                } 
                else 
                {
                    // If read returned 0, we're at the end of the file, so decrement the operations count.
                    ops_in_progress--;
                }
            }
        }
    }

    clock_gettime(CLOCK_MONOTONIC, &finish);
    elapsed = (finish.tv_sec - start.tv_sec);
    elapsed += (finish.tv_nsec - start.tv_nsec) / 1000000000.0;

    close(source_fd);
    close(destination_fd);
    free(buffer);

    printf("\nCopying was completed in %f seconds\n", elapsed);

    return 0;
}
