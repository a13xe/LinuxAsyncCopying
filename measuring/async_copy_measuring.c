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


void aio_read_setup(struct aiocb *aiocbp, int fd, off_t offset, volatile void *buf, size_t size) 
{
    memset(aiocbp, 0, sizeof(struct aiocb));
    aiocbp->aio_fildes = fd;
    aiocbp->aio_buf = buf;
    aiocbp->aio_nbytes = size;
    aiocbp->aio_offset = offset;
    if (aio_read(aiocbp) == -1) 
    {
        perror("aio_read");
        exit(EXIT_FAILURE);
    }
}


void aio_write_setup(struct aiocb *aiocbp, int fd, off_t offset, volatile void *buf, size_t size) 
{
    memset(aiocbp, 0, sizeof(struct aiocb));
    aiocbp->aio_fildes = fd;
    aiocbp->aio_buf = buf;
    aiocbp->aio_nbytes = size;
    aiocbp->aio_offset = offset;
    if (aio_write(aiocbp) == -1) 
    {
        perror("aio_write");
        exit(EXIT_FAILURE);
    }
}


void wait_for_aio_operations(struct aiocb *aiocbp_list, int num_ops) 
{
    for (int i = 0; i < num_ops; i++) {
        while (aio_error(&aiocbp_list[i]) == EINPROGRESS) 
        {
            usleep(1000);
        }
        int ret = aio_return(&aiocbp_list[i]);
        if (ret == -1) 
        {
            perror("aio_return");
            exit(EXIT_FAILURE);
        }
    }
}


int main()
{
    char source_path[256];
    char destination_path[256];
    size_t block_size;

    printf("Enter the block size in KB for copying: ");
    scanf("%zu", &block_size);
    block_size *= 1024; // Convert the block size from KB to bytes.

    size_t block_sizes[] = {block_size, block_size*2, block_size*3, block_size*4, block_size*5, block_size*6, block_size*7, block_size*8};
    int operations[] = {1, 2, 4, 8, 12, 16, 32, 64};
    double execution_times[8][8]; // 8 operations x 8 block sizes
    FILE *file = fopen("execution_times.csv", "w");
    if (file == NULL) 
    {
        perror("fopen");
        exit(EXIT_FAILURE);
    }
    fprintf(file, "BlockSize(KB), 1 Op, 2 Ops, 4 Ops, 8 Ops, 12 Ops, 16 Ops, 32 Ops, 64 Ops\n");
    
    printf("Enter the source file path: ");
    scanf("%255s", source_path);

    printf("Enter the destination file path: ");
    scanf("%255s", destination_path);

    for (int bs_index = 0; bs_index < 8; bs_index++) 
    {
        size_t block_size = block_sizes[bs_index];

        fprintf(file, "%lu", (bs_index++)*block_size / 1024); // Start of a new line in CSV

        for (int op_index = 0; op_index < 8; op_index++) 
        {
            int num_async_ops = operations[op_index];
            int source_fd = open(source_path, O_RDONLY);
            int destination_fd = open(destination_path, O_WRONLY | O_CREAT, 0644);
            char *buffer = (char *)malloc(block_size * num_async_ops);
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

            fprintf(file, ", %f", elapsed); // Writing each execution time to the CSV

            close(source_fd);
            close(destination_fd);
            free(buffer);
        }
        fprintf(file, "\n"); // End of the current line (block size iteration)
    }

    fclose(file);

    printf("\nExecution Times for Different Numbers of Operations and Block Sizes:\n");
    for (int i = 0; i < 8; i++) 
    {
        for (int j = 0; j < 8; j++) 
        {
            printf("%d operations with %lu KB block size: %f seconds\n", operations[i], block_sizes[j] / 1024 / block_size, execution_times[i][j]);
        }
    }

    return 0;
}
