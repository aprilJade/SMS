LINE_CNT=$(command ps -aux | grep ./bin/agent | wc -l)
VALUE=0
MAX_POINT_CNT=1

if [ ${LINE_CNT} -eq 1 ]
then
    echo SMS: Agent is not working.
else
    PID=$(command ps -aux | grep ./bin/agent | head -1 | tr -s ' ' | cut -d ' ' -f 2)
    kill -15 $PID
    while :
    do
        LINE_CNT=$(command ps -aux | grep ./bin/agent | wc -l)
        if [ ${LINE_CNT} -eq 1 ]
        then
            break
        fi
        printf "\033[1A"
        printf "\033[K"  
        printf "SMS: Terminating agent"
        while :
        do
            if  [ ${VALUE} -eq ${MAX_POINT_CNT} ]
            then
                break
            else
                VALUE=$((VALUE+1))  
                printf "."
            fi
        done
        VALUE=0
        MAX_POINT_CNT=$((MAX_POINT_CNT%5))
        MAX_POINT_CNT=$((MAX_POINT_CNT+1))
        printf "\n"
        sleep 1
    done
    echo SMS: Terminate SMS agent successfully.
fi
