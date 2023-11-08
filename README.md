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

- To compile the program, use the following command:

```bash
git clone https://github.com/a13xe/LinuxAsyncCopying.git
cd LinuxAsyncCopying
gcc -o async_copy async_copy.c -lrt
./async_copy
```

- The program will prompt you for these inputs (Example):
  
<div align="center">
<img width=98% alt="Screenshot" src="https://github.com/a13xe/LinuxAsyncCopying/assets/77492646/769d987d-0049-4df3-b9f6-2087467a48e0"/>
</div>


## :spiral_notepad: Notes

- The block size should be a multiple of the filesystem's block size for optimal performance.
- The number of asynchronous operations is limited by system resources and the defined MAX_IO_OPERATIONS in the program.
- The program performs a simple form of error handling and should be extended for more robust production use.
