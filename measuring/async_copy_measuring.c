#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <errno.h>
#include <unistd.h>
#include <string.h>
#include <aio.h>
#include <time.h>

#define MAX_IO_OPERATIONS 64
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
    struct aiocb *aiocbp_array[num_ops];
    for (int i = 0; i < num_ops; ++i) 
    {
        aiocbp_array[i] = &aiocbp_list[i];
    }
    const struct timespec timeout = {1, 0}; // 1-second timeout
    aio_suspend((const struct aiocb *const *)aiocbp_array, num_ops, &timeout);
}

// -------------------------------------------------------------------------------------------------------
// MAIN Function
// -------------------------------------------------------------------------------------------------------
int main() 
{
    char source_path[256];
    char destination_path[256];
    size_t block_sizes[] = {4096, 4096*2, 4096*4, 4096*8, 4096*12, 4096*16, 4096*32};
    int operations[] = {1, 2, 4, 8, 12, 16};
    double execution_times[6][7]; 

    printf("Enter the source file path: ");
    scanf("%255s", source_path);

    printf("Enter the destination file path: ");
    scanf("%255s", destination_path);

    FILE *file = fopen("execution_times.csv", "w");
    if (file == NULL) 
    {
        perror("fopen");
        exit(EXIT_FAILURE);
    }

    fprintf(file, "BlockSize(KB), 1 Op, 2 Ops, 4 Ops, 8 Ops, 12 Ops, 16 Ops\n");

    for (int bs_index = 0; bs_index < 7; bs_index++) 
    {
        size_t block_size = block_sizes[bs_index];
        fprintf(file, "%lu", block_size / 1024); 

        for (int op_index = 0; op_index < 6; op_index++) 
        {
            int num_async_ops = operations[op_index];
            int source_fd = open(source_path, O_RDONLY);
            int destination_fd = open(destination_path, O_WRONLY | O_CREAT, 0644);
            char *buffer = (char *)malloc(block_size * num_async_ops);
            if (!buffer) 
            {
                perror("malloc");
                exit(EXIT_FAILURE);
            }

            off_t offset = 0;
            struct timespec start, finish;
            double elapsed;

            clock_gettime(CLOCK_MONOTONIC, &start);

            for (int i = 0; i < num_async_ops; i++) 
            {
                aio_read_setup(&aiocb_list[i], source_fd, offset, buffer + (i * block_size), block_size);
                offset += block_size;
            }
            
            int ops_in_progress = num_async_ops;
            while (ops_in_progress > 0) 
            {
                wait_for_aio_operations(aiocb_list, num_async_ops);
                for (int i = 0; i < num_async_ops; i++) 
                {
                    ssize_t bytes_read = aio_return(&aiocb_list[i]);
                    if (bytes_read > 0) 
                    {
                        aio_write_setup(&aiocb_list[i], destination_fd, aiocb_list[i].aio_offset, (void *)aiocb_list[i].aio_buf, bytes_read);
                    } 
                    else 
                    {
                        ops_in_progress--;
                    }
                }
                wait_for_aio_operations(aiocb_list, num_async_ops);
                for (int i = 0; i < num_async_ops; i++) 
                {
                    ssize_t bytes_written = aio_return(&aiocb_list[i]);
                    if (bytes_written > 0) 
                    {
                        aio_read_setup(&aiocb_list[i], source_fd, offset, buffer + (i * block_size), block_size);
                        offset += block_size;
                    }
                }
            }
            clock_gettime(CLOCK_MONOTONIC, &finish);
            elapsed = (finish.tv_sec - start.tv_sec);
            elapsed += (finish.tv_nsec - start.tv_nsec) / 1000000000.0;
            execution_times[op_index][bs_index] = elapsed;
            fprintf(file, ", %f", elapsed); 
            close(source_fd);
            close(destination_fd);
            free(buffer);
        }
        fprintf(file, "\n"); 
    }
    fclose(file);
    printf("\nExecution times have been saved to 'execution_times.csv'\n");

    return 0;
}
