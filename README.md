# Asynchronous File Copying for Linux

## :feather: Features

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
Compile the executable:
<td>

```bash
gcc -o async_copy async_copy.c -lrt
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
<img width=98% alt="Screenshot" src="https://github.com/a13xe/LinuxAsyncCopying/assets/77492646/769d987d-0049-4df3-b9f6-2087467a48e0"/>
</div>


## :spiral_notepad: Notes

- The block size should be a multiple of the filesystem's block size for optimal performance.
- The number of asynchronous operations is limited by system resources and the defined MAX_IO_OPERATIONS in the program.
- The program performs a simple form of error handling and should be extended for more robust production use.
