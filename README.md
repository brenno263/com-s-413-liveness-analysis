# Hydrogen: Liveness Analysis

Table of Contents
=================

* [Introduction:](#introduction)
* [Building Hydrogen](#building-hydrogen)
* [Using Hydrogen](#using-hydrogen)
* [Dependencies](#dependencies)

## Introduction:
For this project, our group modified Hydrogen to perform liveness analysis rather than constructing MVICFGs, its original purpose. The bulk of our original work can be found in `Liveness.cpp`.

### Building Hydrogen
1) Before building the project, make sure the [dependencies](#dependencies) are met. You can also make use of the
 [docker image](https://hub.docker.com/r/ashwinkj/hydrogen_env), where the environment is already set up for you.
2) Clone `Hydrogen` from GitHub. If you are using the Docker, you can clone it into `/home/Hydrogen/MVICFG` folder.
3) Compile `Hydrogen` with the help of `CMakeLists.txt`. You can also use `GNU Make`, if that is the preferred method.
4) Assuming you are using the Docker and Ninja, the steps would be like below. But first [install](https://docs.docker.com/install/) Docker using the recommended method for your system.
```sh
# Download and run the Docker from your system.
$ docker run -it --name Hydrogen_Env ashwinkj/hydrogen_env
# The above command will put you inside the Docker Container.
$$ git clone https://github.com/iowastateuniversity-programanalysis/hydrogen /home/Hydrogen/MVICFG
$$ cd /home/Hydrogen/MVICFG
$$ mkdir BuildNinja
$$ cmake -B BuildNinja -G Ninja .
$$ cd BuildNinja
$$ ninja
```

### Using the Program
Open the Hydrogen docker image. Inside of the Hydrogen docker container, run the following commands:
```sh
$$ cd /home/Hydrogen
$$ git clone https://github.com/brenno263/com-s-413-liveness-analysis.git
$$ cd com-s-413-liveness-analysis
```
We include a build.sh script to generate the bytecode for our test program, compile Hydrogen, and run the program. To use this, you can simply run:
```sh
	$$ ./build.sh
```
If you would like to do this yourself or use your own C program, you can do the following:
```sh
$$ mkdir BuildNinja
$$ cmake -B BuildNinja -G Ninja .
$$ cd BuildNinja
$$ ninja
```
To compile your C program into LLVM bytecode, run:
```sh
	$$ clang -c -O0 -Xclang -disable-O0-optnone -g -fno-discard-value-names -emit-llvm -S /path/to/your/program.c -o /path/to/your/program.bc
```
Finally, to run the program run:
```sh
	$$ ./Hydrogen.out /path/to/your/program.bc :: /path/to/your/program.c
```
For our test program, you can run:
```sh
	$$ clang -c -O0 -Xclang -disable-O0-optnone -g -fno-discard-value-names -emit-llvm -S ../TestPrograms/Prog.c -o ../TestPrograms/Prog.bc
	$$ ./Hydrogen.out ../TestPrograms/Prog.bc :: ../TestPrograms/Prog.c
```

## Dependencies
Hydrogen depends on the `LLVM Framework` and `Boost Libraries`. Roughly, the following are required for Hydrogen to
 build properly

| Program | Version |
|---------|---------|
| `Clang` | 8.0     |
| `LLVM`  | 8.0     |
| `Boost` | 1.69    |
| `Cmake` | 3.14    |
| `Ninja` | 1.9     |

While slightly older versions for `Cmake` and `Ninja` can be used without any problem, using older versions of
 `LLVM Framework` and `Boost` can have unwanted consequences and may even result in build failure.

