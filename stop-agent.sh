CNT=$(command ps -aux | grep ./bin/agent | wc -l)
if [ ${CNT} -eq 1 ]
then
    echo SMS: Agent is not working.
else
    echo SMS: Terminate SMS agent.
    PID=$(command ps -aux | grep ./bin/agent | head -1 | cut -d ' ' -f 2)
    kill -15 $PID
fi
