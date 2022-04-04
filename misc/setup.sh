# Instructions for Docker to run a container
docker build -t vsomeip-fuzzing .
docker run -it --rm --name vsomeip-fuzz vsomeip-fuzzing

# Instructions for Docker to enter a container
docker run -it -e "TERM=xterm-256color" --name vsomeip-fuzz vsomeip-fuzzing bash
docker start vsomeip-fuzz && docker exec -it vsomeip-fuzz bash

# Instructions for AFL++ inside a container
afl-fuzz -D -i afl_input/ -o afl_output/ ./fuzzing @@
afl-cov -d afl_output/ --coverage-cmd "./fuzzing AFL_FILE" --code-dir ../ --overwrite

# Instructions for AFL++ outside a container
docker run -t -d --name vsomeip-fuzz vsomeip-fuzzing bash
docker exec -it vsomeip-fuzz ../misc/runtime.sh -fuzz 10
docker exec -it vsomeip-fuzz ../misc/runtime.sh -report
docker cp vsomeip-fuzz:/src/vsomeip-fuzzing/build/afl_output/cov .
