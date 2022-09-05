CNT=$(command ps -aux | grep ./bin/agent | wc -l)
if [ ${CNT} -eq 1 ]
then
    echo SMS: Agent is not working.
else
    echo SMS: Terminating agent....
    PID=$(command ps -aux | grep ./bin/agent | head -1 | tr -s ' ' | cut -d ' ' -f 2)
    kill -15 $PID
    while :
    do
        CNT=$(command ps -aux | grep ./bin/agent | wc -l)
        if [ ${CNT} -eq 1 ]
        then
            break
        fi
        sleep 1
    done
    echo SMS: Terminate SMS agent successfully.
fi
