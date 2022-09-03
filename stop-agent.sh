PID=$(command ps -aux | grep ./bin/agent | head -1 | cut -d ' ' -f 2)
kill -15 $PID