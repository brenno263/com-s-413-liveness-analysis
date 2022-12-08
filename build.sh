cd TestPrograms
clang -c -O0 -Xclang -disable-O0-optnone -g -fno-discard-value-names -emit-llvm -S Prog.c -o Prog.bc
cd ..


rm -rf ./ninja/
mkdir ninja
cmake -B ninja -G Ninja .
cd ninja
ninja
# cat ../TestPrograms/Correct/Prog.c
./Hydrogen.out ../TestPrograms/Prog.bc :: ../TestPrograms/Prog.c
cd ..
chmod -R 777 ninja
