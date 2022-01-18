# Instructions for Docker to build a container
docker build -t vsomeip-testing .
docker run -it -e "TERM=xterm-256color" --name vsomeip-testing vsomeip-testing
docker start vsomeip-testing
docker exec -it vsomeip-testing bash
docker cp vsomeip-testing:/src/vsomeip-testing/build/afl_output/cov .

# Instructions for AFL++ inside a container
afl-fuzz -D -i afl_input/ -o afl_output/ ./fuzzing @@
afl-cov -d afl_output/ --coverage-cmd "./fuzzing AFL_FILE" --code-dir ../ --overwrite
