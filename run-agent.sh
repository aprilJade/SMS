export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:/home/apriljade/Documents/repository/SMS/SMSutils
# -p [period] -c [period] -m [period] -n [period]
# The [period] is the collection period and is in milliseconds.
# If you want collect CPU info every 500ms, memory info every 500ms, network info every 500ms and
# process info every 3000ms, then start below command
# ./agent/agent -c 500 -m 500 -n 500 -p 3000

./agent/agent -p 10000 -c 10000 -m 10000 -n 10000