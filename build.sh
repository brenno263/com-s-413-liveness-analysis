function clang-llvm() {
	clang -c -O0 -Xclang -disable-O0-optnone -g -fno-discard-value-names -emit-llvm -S $1.c -o $1.bc
}

# meant to be run from within the ninja directory
function run() {
	echo "Running Liveness Analysis on test case $1"
	./Hydrogen.out ../TestPrograms/$1.bc :: ../TestPrograms/$1.c
}


cd TestPrograms
clang-llvm unused_var
clang-llvm unset_var
clang-llvm changed_var
clang-llvm unchanged_var
clang-llvm used_func
clang-llvm dead_func
cd ..


rm -rf ./ninja/
mkdir ninja
cmake -B ninja -G Ninja .
cd ninja
ninja

run unused_var
run unset_var
run changed_var
run unchanged_var
run used_func
run dead_func
cd ..
chmod -R 777 ninja
