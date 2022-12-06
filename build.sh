cd TestPrograms/Buggy
clang -c -O0 -Xclang -disable-O0-optnone -g -emit-llvm -S Prog.c -o ProgV1.bc
cd ../Buggy2
clang -c -O0 -Xclang -disable-O0-optnone -g -emit-llvm -S Prog.c -o ProgV2.bc
cd ../Correct
clang -c -O0 -Xclang -disable-O0-optnone -g -emit-llvm -S Prog.c -o ProgV3.bc
cd ../..


rm -rf ./ninja/
mkdir ninja
cmake -B ninja -G Ninja .
cd ninja
ninja
# cat ../TestPrograms/Correct/Prog.c
./Hydrogen.out ../TestPrograms/Buggy/ProgV1.bc ../TestPrograms/Buggy2/ProgV2.bc ../TestPrograms/Correct/ProgV3.bc :: ../TestPrograms/Buggy/Prog.c :: ../TestPrograms/Buggy2/Prog.c :: ../TestPrograms/Correct/Prog.c
cd ..
chmod -R 777 ninja
