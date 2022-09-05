CNT=$(command ps -aux | grep .bin/server | wc -l)
if [ ${CNT} -eq 1 ]
then
    echo SMS: Server is not working.
else
    echo SMS: Terminate SMS server.
    PID=$(command ps -aux | grep .bin/server | head -1 | tr -s ' ' | cut -d ' ' -f 2)
    kill -15 $PID
fi

