export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:$(pwd)/bin

CNT=$(command ps -aux | grep .bin/server | wc -l)
if [ ${CNT} -eq 2 ]
then
    echo SMS: Server is already working.
else
    echo SMS: Start SMS server.
    ./bin/server ./server.conf
fi
