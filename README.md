# Asynchronous File Copying for Linux

## :bug: Features

- Asynchronous I/O operations for non-blocking file copying.
- Customizable block size for copying, set by the user.
- The number of simultaneous asynchronous operations can be specified by the user.
- Timing of the copy operation to determine the duration of the process.

## :gear: Requirements

- A Linux environment with support for POSIX AIO.
- GCC or another compatible C compiler.

```bash
sudo apt update
sudo apt install gcc git -y
```

## :hammer_and_wrench: Compilation and Usage

### 1. To compile the program, use the following commands:


<table>
<tr>
<td>
Clone the repo:
<td>

```bash
git clone https://github.com/a13xe/LinuxAsyncCopying.git
```

<tr>
<td>
Navigate to the directory:
<td>

```bash
cd LinuxAsyncCopying
```

<tr>
<td>
Compile the executable (choose one):
<td>

```bash
gcc -o async_copy async_copy_aio_error.c -lrt
```
```bash
gcc -o async_copy async_copy_aio_suspend.c -lrt
```

<tr>
<td>
Execute the program:
<td>

```bash
./async_copy
```
</table>


### 2. The program will prompt you for these inputs (Example):
  
<div align="center">
<img width=49% alt="Screenshot" src="https://github.com/a13xe/LinuxAsyncCopying/assets/77492646/d76759c6-17ad-4daa-8cc4-f0327727eaea"/>
<img width=49% alt="Screenshot" src="https://github.com/a13xe/LinuxAsyncCopying/assets/77492646/b3bbffe6-e046-4c25-9590-addb3d4e06af"/>
</div>

------------------------------------------------------------------------------------------------------------------

## :bookmark: Comparing Performance: aio_suspend vs. aio_error

- The performance difference between using `aio_suspend` and active waiting with `aio_error` can be explained by the differences in waiting mechanisms and processing of asynchronous operations.

- When you use `aio_suspend`, the program pauses and waits for a notification from the kernel about the completion of at least one of the asynchronous operations, which can lead to some delay since the program "wakes up" only after receiving such a notification. This means there can be some time between the completion of the operation and the resumption of the program's work during which the CPU is not occupied with processing other copying operations.

- On the other hand, active waiting with `aio_error` leads to the program constantly checking the status of each asynchronous operation in a loop. Although this might seem less efficient in terms of CPU usage, such an approach can allow for a quicker response to the completion of operations, especially if the operations finish at different speeds. This can lead to an earlier start of new read or write operations, which, in turn, can speed up the overall copying process.

- It is also important to consider that the performance of asynchronous I/O can greatly depend on the system characteristics, such as disk performance, system load, block size, and the number of concurrent asynchronous operations. In some cases, active waiting might be more preferable if it allows for a more efficient use of the available I/O bandwidth, despite the higher CPU consumption.

- Ultimately, the choice between `aio_suspend` and active waiting with `aio_error` should be based on the specific performance and resource requirements of your application, as well as the characteristics of the target system. In some cases, it might be useful to conduct your own performance testing to determine the most efficient approach.

- In my case, when using aio_suspend, a 4 GB file is copied in 16 seconds using an 8 KB block size and 8 operations, whereas using aio_error for active waiting, the same operation takes only 9 seconds
