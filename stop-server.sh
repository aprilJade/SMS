CNT=$(command ps -aux | grep .bin/server | wc -l)
if [ ${CNT} -eq 1 ]
then
    echo SMS: Server is not working.
else
    echo SMS: Terminating SMS server...
    PID=$(command ps -aux | grep .bin/server | head -1 | tr -s ' ' | cut -d ' ' -f 2)
    kill -15 $PID
    while :
    do
        CNT=$(command ps -aux | grep ./bin/server | wc -l)
        if [ ${CNT} -eq 1 ]
        then
            break
        fi
        sleep 1
    done
    echo SMS: Terminate SMS server successfully.
fi

