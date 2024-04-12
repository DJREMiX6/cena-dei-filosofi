#use "chmod a+rwx ./build-and-run.sh" to give the script the necessary permissions to be executed
rm -r ./out ; \
mkdir ./out/ && \
gcc ./src/main.c ./src/argsint.c -I ./src/ -o ./out/Filosofi