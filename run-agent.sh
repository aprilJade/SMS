# If you want remove below 2 lines, you must register necessary librarys to standard library path.
# Such as, /usr/lib
export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:$(pwd)/bin

# -p [period] -c [period] -m [period] -n [period] -H [host:port]
# The [period] is the collection period and is in milliseconds.
# If you want collect CPU info every 500ms, memory info every 500ms, network info every 500ms and
# process info every 3000ms, then start below command
# ./agent/agent -c 500 -m 500 -n 500 -p 3000

LD_PRELOAD=./bin/libhook_module.so ./bin/agent ./agent.conf
