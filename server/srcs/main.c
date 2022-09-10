#include <stdio.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <arpa/inet.h>
#include <string.h>
#include <unistd.h>
#include <assert.h>
#include <stdlib.h>
#include <pthread.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include "receiveRoutine.h"
#include "workerRoutine.h"
#include "pgWrapper.h"
#include "confParser.h"
#include "udpPacket.h"

#define CONNECTION_COUNT 1024
#define MAX_WORKER_COUNT 16
#define DEFAULT_PORT 4242

static const char* const strSignal[] = {
	[SIGBUS] = "SIGBUS",
	[SIGABRT] = "SIGABRT",
	[SIGFPE] = "SIGFPE",
	[SIGQUIT] = "SIGQUIT",
	[SIGSEGV] = "SIGSEGV",
	[SIGINT] = "SIGINT",
	[SIGILL] = "SIGILL",
	[SIGSYS] = "SIGSYS",
	[SIGTERM] = "SIGTERM",
	[SIGKILL] = "SIGKILL",
};

bool g_turnOff = false;
const Logger* g_logger;
int udpSockFd;
pthread_t udpTid;
pthread_t* workerId;
Queue* g_queue;
int g_stderrFd = 2;
pthread_mutex_t g_clientCntLock = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t g_clientCntCond = PTHREAD_COND_INITIALIZER;
unsigned int g_clientCnt = 0;

void HandleSignal(int signo)
{
	char logMsg[512];

	if (signo == SIGQUIT || signo == SIGTERM)
	{
        g_turnOff = true;
        pthread_mutex_lock(&g_clientCntLock);
        pthread_cond_broadcast(&g_clientCntCond);
        pthread_mutex_unlock(&g_clientCntLock);
        for (int i = 0; i < 4; i++)
            pthread_join(workerId[i], NULL);
		Log(g_logger, LOG_INFO, "Server is terminated");
	}
	else
	{
		sprintf(logMsg, "Server is aborted: %s", strSignal[signo]);
		Log(g_logger, LOG_FATAL, logMsg);
        pthread_mutex_lock(&g_clientCntLock);
        pthread_cond_broadcast(&g_clientCntCond);
        pthread_mutex_unlock(&g_clientCntLock);
		char logPathBuf[128];
		GenLogFileFullPath(g_logger->logPath, logPathBuf);

		sprintf(logMsg, "SMS: Server is aborted. Check below log.\n%s\n", logPathBuf);
		write(g_stderrFd, logMsg, strlen(logMsg));
	    exit(signo);
	}

	exit(signo);
}

int OpenSocket(short port)
{
    struct sockaddr_in servAddr;
    int servFd = socket(PF_INET, SOCK_STREAM, 0);
    if (servFd == -1)
        return -1;

    memset(&servAddr, 0, sizeof(servAddr));
    servAddr.sin_addr.s_addr = htonl(INADDR_ANY);
    servAddr.sin_port = htons(port);
    servAddr.sin_family = AF_INET;

    if (bind(servFd, (struct sockaddr*)&servAddr, sizeof(servAddr)) == -1)
    {
        close(servFd);
        return -1;
    }
    return servFd;
}

static ulong ConvertToBytesFromLargeUnit(char* str)
{
    ulong result = atol(str);
    int i = 0;
    while(str[i] >= '0' && str[i] <= '9')
        i++;
    
    if (i == strlen(str))
        return result;
    switch (str[i])
    {
    case 'k':
        result *= 1024;
        break;
    case 'M':
        result *= 1024 * 1024;
        break;
    case 'G':
        result *= 1024 * 1024 * 1024;
        break;
    }
    return result;
}

static SThreshold GetThresholds(SHashTable* options)
{
    char* value;
    float fTmp;
    uint uTmp;
    SThreshold ret = { 100, 100, 100, 3000, 3000 };

    if ((value = GetValueByKey(CONF_KEY_CPU_UTILIAZATION_THRESHOLD, options)) != NULL)
    {
        fTmp = atof(value);
        if (fTmp >= 0 && fTmp <= 100.0)
            ret.cpuUtilization = fTmp;    
    }

    if ((value = GetValueByKey(CONF_KEY_MEM_USAGE_THRESHOLD, options)) != NULL)
    {  
        fTmp = atof(value);
        if (fTmp >= 0 && fTmp <= 100.0)
            ret.memUsage = fTmp;   
    }

    if ((value = GetValueByKey(CONF_KEY_SWAP_USAGE_THRESHOLD, options)) != NULL)
    {  
        fTmp = atof(value);
        if (fTmp >= 0 && fTmp <= 100.0)
            ret.swapUsage = fTmp;   
    }

    if ((value = GetValueByKey(CONF_KEY_SEND_BYTES_THRESHOLD, options)) != NULL)
    {  
        if (atoi(value) >= 0)
            ret.sendBytes = ConvertToBytesFromLargeUnit(value);   
    }

    if ((value = GetValueByKey(CONF_KEY_RECV_BYTES_THRESHOLD, options)) != NULL)
    {  
        if (atoi(value) >= 0)
            ret.recvBytes = ConvertToBytesFromLargeUnit(value);   
    }

    return ret;
}

void CreateWorker(int workerCount, SPgWrapper* db, SHashTable* options)
{
    SWorkerParam* param;
    SThreshold threshold = GetThresholds(options);
    workerId = (pthread_t*)malloc(sizeof(pthread_t) * workerCount);

    if (db->connected == false)
        Log(g_logger, LOG_FATAL, "PostgreSQL connection failed");
    
    for (int i = 0; i < workerCount; i++)
    {
        param = (SWorkerParam*)malloc(sizeof(SWorkerParam));
        param->workerId = i;
        param->db = db;
        param->threshold = threshold;
        pthread_create(&workerId[i], NULL, WorkerRoutine, param);
    }
}

Logger* GenLogger(SHashTable* options)
{
	char* logPath;
	char* logLevel;
	Logger* logger;

	if ((logPath = GetValueByKey(CONF_KEY_LOG_PATH, options)) == NULL)
		logPath = "./server";
	if ((logLevel = GetValueByKey(CONF_KEY_LOG_LEVEL, options)) != NULL)
	{
		if (strcmp(logLevel, "default") == 0)
			logger = NewLogger(logPath, LOG_INFO);
	}
	logger = NewLogger(logPath, LOG_DEBUG);
	return logger;
}

void* UdpRoutine(void* param)
{
    SPgWrapper* db = (SPgWrapper*)param;

    udpSockFd = socket(PF_INET, SOCK_DGRAM, 0);
    if (udpSockFd < 0)
        return 0;
    struct sockaddr_in udpAddr;
    memset(&udpAddr, 0, sizeof(udpAddr));

    udpAddr.sin_family = AF_INET;
    udpAddr.sin_addr.s_addr = htonl(INADDR_ANY);
    udpAddr.sin_port = htons(4343);

    if (bind(udpSockFd, (struct sockaddr*)&udpAddr, sizeof(udpAddr)) < 0)
        return 0;
    
    int readSize;
    char buf[1024];
    char logMsg[256];
    struct sockaddr_in udpClientAddr;
    socklen_t len;
    SPrefixPkt* prevPkt;
    SPostfixPkt* postPkt;

    sprintf(logMsg, "Start UDP receiver");
    Log(g_logger, LOG_INFO, logMsg);
    char* strInsert = "INSERT INTO udp_informations (agent_id, measurement_time, process_name, pid, max_send_bytes, send_bytes_avg, max_elapse_time, elapse_time_avg) VALUES";
    char sql[512];
    while (1)
    {
        if (g_turnOff)
            break;
        if (!db->connected)
        {
            sprintf(logMsg, "Wait DB connection...");
            Log(g_logger, LOG_INFO, logMsg);
            pthread_mutex_lock(&db->lock);
            pthread_cond_wait(&db->cond, &db->lock);
            pthread_mutex_unlock(&db->lock);
            sprintf(logMsg, "PostgreSQL is connected");
            Log(g_logger, LOG_INFO, logMsg);
        }

        if ((readSize = recvfrom(udpSockFd, buf, sizeof(SPrefixPkt), 0, (struct sockaddr*)&udpClientAddr, &len)) < 0)
        {
            sprintf(logMsg, "Fail to receive UDP packet");
            Log(g_logger, LOG_ERROR, logMsg);
            continue;
        }
        buf[readSize];

        if (readSize != sizeof(SPrefixPkt))
        {
            // TODO: handle packet loss..
            continue;
        }
        
        prevPkt = (SPrefixPkt*)buf;
        // TODO: handle prevPkt...

        if ((readSize = recvfrom(udpSockFd, buf, sizeof(SPostfixPkt), 0, (struct sockaddr*)&udpClientAddr, &len)) < 0)
        {
            sprintf(logMsg, "Fail to receive UDP packet");
            Log(g_logger, LOG_ERROR, logMsg);  
            continue;
        }
        buf[readSize];

        if (readSize != sizeof(SPostfixPkt))
        {
            // TODO: handle packet loss...
            continue;
        }

        postPkt = (SPostfixPkt*)buf;
        struct tm* ts;
        ts = localtime(&postPkt->measurementTime);
        sprintf(sql, "%s (\'%s\', \'%04d-%02d-%02d %02d:%02d:%02d\', \'%s\', %u, %u, %f, %lu, %f);",
            strInsert,
            postPkt->agentId,
            ts->tm_year + 1900, ts->tm_mon + 1, ts->tm_mday,
            ts->tm_hour, ts->tm_min, ts->tm_sec,
            postPkt->processName,
            postPkt->pid,
            postPkt->maxSendBytes,
            postPkt->sendBytesAvg,
            postPkt->maxElapseTime,
            postPkt->elapseTimeAvg);

        if (Query(db, sql) == -1)
        {
            sprintf(sql, "Failed to store in DB: UDP");
            Log(g_logger, LOG_ERROR, sql);
            continue;
        }
    }

    sprintf(logMsg, "End UDP receiver");
    Log(g_logger, LOG_INFO, logMsg);
}

int main(int argc, char** argv)
{
    if (argc != 2)
	{
		fprintf(stderr, "ERROR: you must input conf file path\n");
		return EXIT_FAILURE;
	}

	SHashTable* options = NewHashTable();
    int ret;
    
	if ((ret = ParseConf(argv[1], options)) != CONF_NO_ERROR)
	{
		fprintf(stderr, "conf error: %d\n", ret);
		exit(EXIT_FAILURE);
	}

    char* tmp;
    if ((tmp = GetValueByKey(CONF_KEY_RUN_AS_DAEMON, options)) != NULL)
	{
		if (strcmp(tmp, "true") == 0)
		{
			pid_t pid = fork();

			if (pid == -1)
			{
				fprintf(stderr, "ERROR: failed to fork: %s\n", strerror(errno));
				exit(EXIT_FAILURE);
			}

			if (pid != 0)
				exit(EXIT_SUCCESS);
			
			signal(SIGHUP, SIG_IGN);
			close(STDIN_FILENO);
			close(STDOUT_FILENO);
            dup2(STDERR_FILENO, g_stderrFd);
			close(STDERR_FILENO);
			chdir("/");
			setsid();
		}
	}

    signal(SIGBUS, HandleSignal);	// bus error
	signal(SIGABRT, HandleSignal);	// abort signal
	signal(SIGFPE, HandleSignal);	// floating point error
	signal(SIGQUIT, HandleSignal);	// quit signal
	signal(SIGSEGV, HandleSignal);  // segmentation fault
	signal(SIGINT, HandleSignal);	// interrupted
	signal(SIGILL, HandleSignal);	// illegal instruction
	signal(SIGSYS, HandleSignal);	// system call error
	signal(SIGTERM, HandleSignal);	// terminate signalr
	signal(SIGKILL, HandleSignal);	// terminate signal

    tmp = GetValueByKey(CONF_KEY_LISTEN_PORT, options);
    unsigned short port = 4242;
    if (tmp != NULL)
        port = (unsigned short)atoi(tmp);
	g_logger = GenLogger(options);
    char logMsg[256];

    sprintf(logMsg, "Server loaded: %d", getpid());
    Log(g_logger, LOG_INFO, logMsg);
    
    SPgWrapper* db = NewPgWrapper("dbname = postgres");
    pthread_create(&udpTid, NULL, UdpRoutine, db);
    
    int servFd, clientFd;
    struct sockaddr_in clientAddr;
    if ((servFd = OpenSocket(port)) == -1)
    {
        sprintf(logMsg, "Socket is not opened: %s", strerror(errno));
        Log(g_logger, LOG_FATAL, "Socket is not opened.");

        char logPath[128];
		GenLogFileFullPath(g_logger->logPath, logPath);
		sprintf(logMsg, "SMS: Socket is not opened. Check below log.\n%s\n", logPath);
		write(g_stderrFd, logMsg, strlen(logMsg));
        exit(EXIT_FAILURE);
    }

    socklen_t len = sizeof(clientAddr);
    pthread_t tid;
    SReceiveParam* param;
    g_queue = NewQueue();
    tmp = GetValueByKey(CONF_KEY_WORKER_COUNT, options);
    int workerCount = 2;
    if (tmp != NULL)
        workerCount = atoi(tmp);
    CreateWorker(workerCount, db, options);

    while (1)
    {
        if (listen(servFd, CONNECTION_COUNT) == -1)
        {
            Log(g_logger, LOG_FATAL, "Failed listening");
            exit(EXIT_FAILURE);
        }

        sprintf(logMsg, "Wait for connection from client at %d", port);
        Log(g_logger, LOG_INFO, logMsg);

        clientFd = accept(servFd, (struct sockaddr*)&clientAddr, &len);
        if (clientFd == -1)
        {
            Log(g_logger, LOG_FATAL, "Failed to accept connection");
            exit(EXIT_FAILURE);
        }

        param = (SReceiveParam*)malloc(sizeof(SReceiveParam));
        param->clientSock = clientFd;
        param->host = inet_ntoa(clientAddr.sin_addr);
        param->port = ntohs(clientAddr.sin_port);

        if (pthread_create(&tid, NULL, ReceiveRoutine, param) == -1)
        {
            Log(g_logger, LOG_FATAL, "Failed to create receiver");
            close(clientFd);
            continue;
        }
        
        pthread_mutex_lock(&g_clientCntLock);
        g_clientCnt++;
        pthread_cond_broadcast(&g_clientCntCond);
        pthread_mutex_unlock(&g_clientCntLock);
        

        sprintf(logMsg, "Start TCP receiver for %s:%d", param->host, param->port);
        Log(g_logger, LOG_INFO, logMsg);
        pthread_detach(tid);
    }
}   