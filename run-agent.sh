# If you want remove below 2 lines, you must register necessary librarys to standard library path.
# Such as, /usr/lib
export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:$(pwd)/bin

echo SMS: Start SMS agent.
LD_PRELOAD=./bin/libhook_module.so ./bin/agent ./agent.conf

