#!/bin/bash

RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[1;34m'
NC='\033[0m'

if [[ "$1" == "-c" ]]; then
    rm -f ast_*
    rm -f *.class
    rm -f *.o
    rm -f *.hi
    rm -f *.cm*
    echo -e "${GREEN}cleaned${NC}"
    exit 0
fi

files=(ast.c ast.cpp ast.go ast.hs ast.java ast.ml ast.rs ast.zig)

declare -A compilers=(
    [c]=gcc
    [cpp]=g++
    [go]=go
    [hs]=ghc
    [java]=javac
    [ml]=ocamlopt
    [rs]=rustc
    [zig]=zig
)

declare -A commands=(
    [c]='gcc -O2 ast.c -o ast_c'
    [cpp]='g++ -O2 ast.cpp -o ast_cpp'
    [go]='go build -o ast_go ast.go'
    [hs]='ghc -O2 ast.hs -o ast_hs'
    [java]='javac ast.java'
    [ml]='ocamlopt -O3 ast.ml -o ast_ml'
    [rs]='rustc -C opt-level=3 ast.rs -o ast_rs'
    [zig]='zig build-exe -O ReleaseFast ast.zig -femit-bin=ast_zig'
)

missing=0
for ext in "${!compilers[@]}"; do
    if ! command -v "${compilers[$ext]}" >/dev/null 2>&1; then
        echo -e "${RED}missing compiler for .$ext files: ${compilers[$ext]}${NC}"
        missing=1
    fi
done

if [ "$missing" -ne 0 ]; then
    exit 1
fi

tput sc

for file in "${files[@]}"; do
    ext="${file##*.}"
    lang="$ext"
    printf "[${YELLOW}%s${NC}] ${BLUE}compiling...${NC}\n" "$lang"
done

pids=()
i=0
for file in "${files[@]}"; do
    ext="${file##*.}"
    lang="$ext"

    (
        if eval "${commands[$ext]} &> /dev/null"; then
            result="${GREEN}ok${NC}!"
        else
            result="${RED}error${NC}"
        fi

        tput rc
        for ((j=0; j<i; j++)); do tput cud1; done
        tput el
        printf "[${YELLOW}%s${NC}] %b" "$lang" "$result"
        tput rc
    ) &

    pids+=($!)
    ((i++))
done

for pid in "${pids[@]}"; do
    wait "$pid"
done

tput rc
for ((j=0; j<${#files[@]}; j++)); do tput cud1; done

echo -e "${GREEN}build complete${NC}"

rm -f *.o
rm -f *.hi
rm -f *.cm*
