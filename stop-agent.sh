LINE_CNT=$(command ps -aux | grep ./bin/agent | wc -l)
LINE_CNT=$((LINE_CNT-1))

printf "current count of running agent: %d\n" $LINE_CNT
while :
do
    LINE_CNT=$(command ps -aux | grep ./bin/agent | wc -l)
    if [ ${LINE_CNT} -eq 1 ]
    then
        break
    fi
    PID=$(command ps -aux | grep ./bin/agent | head -1 | tr -s ' ' | cut -d ' ' -f 2)
    kill $PID
done
echo SMS: There are no more agents running.
