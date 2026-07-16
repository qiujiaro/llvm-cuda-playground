open -a docker
docker compose up --build

frontend: http://localhost:3000

backend: http://localhost:8000

engine

cmake -S engine -B build/engine
cmake --build build/engine

run
./build/engine/llvm-engine
