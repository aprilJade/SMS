PID=$(command ps -aux | grep .bin/agent | head -1 | cut -d ' ' -f 3)
kill -9 $PID