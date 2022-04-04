# Instructions for Docker to build a container
docker build -t vsomeip-fuzzing .
docker run -it -e "TERM=xterm-256color" --name vsomeip-fuzz vsomeip-fuzzing
docker start vsomeip-fuzz
docker exec -it vsomeip-fuzz bash
docker cp vsomeip-fuzz:/src/vsomeip-fuzzing/build/afl_output/cov .

# Instructions for AFL++ inside a container
afl-fuzz -D -i afl_input/ -o afl_output/ ./fuzzing @@
afl-cov -d afl_output/ --coverage-cmd "./fuzzing AFL_FILE" --code-dir ../ --overwrite
