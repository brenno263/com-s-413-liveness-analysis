function clang-llvm() {
	clang -c -O0 -Xclang -disable-O0-optnone -g -fno-discard-value-names -emit-llvm -S $1.c -o $1.bc
}

# meant to be run from within the ninja directory
function run() {
	./Hydrogen.out ../TestPrograms/$1.bc :: ../TestPrograms/$1.c
}


cd TestPrograms
clang-llvm unused_var
clang-llvm unset_var
cd ..


rm -rf ./ninja/
mkdir ninja
cmake -B ninja -G Ninja .
cd ninja
ninja

run unused_var
run unset_var
cd ..
chmod -R 777 ninja
