# SMS
System Monitoring System의 약자입니다. 대상 OS를 모니터링하여 분석하는 시스템입니다.

# 실행방법

```
주의! Ubuntu 22.04에서만 테스트 된 코드로 다른 환경에서 동작이 보장되지 않습니다.
```

1. 해당 레포지토리를 클론해주세요.
```
git clone https://github.com/aprilJade/SMS.git
```
2. 빌드 후 서버를 실행해주세요.
```
cd SMS
make
sh run-server.sh
```
3. 이후 새로운 터미널을 켜고 agent를 실행해주세요. 
```
sh run-agent.sh
```

# Options
- .conf파일에 어플리케이션의 옵션을 작성합니다. 각 옵션은 다음과 같은 의미입니다.
- 각 옵션은 SMS/include/confParser.h에 정의되어있습니다.

## agent options
|옵션|설명|예시|
|---|---|---|
|ID|Agent의 ID|agent001|
|HOST_ADDRESS|연결할 서버의 IP주소|127.0.0.1|
|HOST_PORT|연결할 서버의 Port번호|4242|
|RUN_AS_DAEMON|Daemon으로 실행할지 여부|true / false|
|LOG_LEVEL|로깅 레벨 설정|default / debug|
|LOG_PATH|로그 저장 경로|/path/to/log|
|RUN_CPU_COLLECTOR|CPU 정보 수집 여부|true / false|
|CPU_COLLECTION_PERIOD|CPU 정보 수집 주기 (ms)|1000 (minimum 500)|
|RUN_MEM_COLLECTOR|Memory 정보 수집 여부|true / false|
|MEM_COLLECTION_PERIOD|Memory 정보 수집 주기 (ms)|1000 (minimum 500)|
|RUN_NET_COLLECTOR|Network 정보 수집 여부|true / false|
|NET_COLLECTION_PERIOD|Network 정보 수집 주기 (ms)|1000 (minimum 500)|
|RUN_PROC_COLLECTOR|Process 정보 수집 여부|true / false|
|PROC_COLLECTION_PERIOD|Process 정보 수집 주기 (ms)|1000 (minimum 500)|
|RUN_DISK_COLLECTOR|Disk 정보 수집 여부|true / false|
|DISK_COLLECTIONS_PERIOD|Disk 정보 수집 주기 (ms)|1000 (minimum 500)|

## server options
|옵션|설명|예시|
|---|---|---|
|LISTEN_PORT|수신할 Port번호|4242|
|MAX_CONN|연결가능한 agent 최대 개수|128|
|WORKER_COUNT|패킷을 처리할 Worker 스레드 개수|4 (maximum 16)|
|RUN_AS_DAEMON|Daemon으로 실행할지 여부|true / false|
|LOG_LEVEL|로깅 레벨 설정|default / debug|
|LOG_PATH|로그 저장 경로|/path/to/log|

# Feedback log
## 2022.08.23 3주차 Feedback
- [ ] Queuing을 위한 Queue접근 시 Lock 사용 비율을 줄이는 방향으로 개선

## 2022.08.16 2주차 Feedback
- [x] ~Process 패킷을 하나로 만들어서 송신~
- [x] ~Process 정보를 한번에 쿼리하기 (auto commit을 안하는 것으로 반복적인 commit을 최소화하는 것으로 해결)~
- [x] ~Log level을 두고 Logging~
- [ ] Agent ID를 추가하여 Agent 식별하기

## 2022.08.09 1주차 Feedback
- [x] ~배열로 구현한 큐를 링크드 리스트로 구현하기(큐의 사이즈가 정적인 것이 큰 문제를 일으킬 수 있음)~
- [x] ~서버 스레드 구조 개편~
  - [x] ~수신스레드를 하나로 만들고 Queuing (비즈니스 로직과 수신을 분리)~
  - [x] ~복수의 워커스레드를 만들어 Queue에서 데이터를 가져와 가공 및 DB저장~
- [ ] 클라이언트가 수천개일때도 생각해서 구현하기


# Todo List
- [ ] 공통
  - [ ] 고도화
    - [ ] agent와 server간의 handshake 구현 (TCP handshake가 아니라 application level의 handshake를 뜻함)
      - [ ] 1. 상수 값 송신(메모리 총량, 디스크 총량, 네트워크 인터페이스 종류, CPU개수 등 시스템 기본 정보)
      - [ ] 2. 가능하다면 인증 기능 추가 => server는 agent 회원 목록을 들고있고 연결 시도하는 agent가 목록에 해당하는 agent인지 검증
    - [ ] delta값과 average값 계산
      - [ ] delta값은 cpu 사용율, 메모리 사용율, 디스크 사용율, 네트워크 송수신 패킷수, 네트워크 송수신 바이트수를 계산 (모든 값은 초당 값으로 계산)
      - [ ] average값은 1시간 단위의 delta값을 측정 (1시간 평균 cpu사용율 등)
  - [ ] 코드 정리 (수시로 반복할 것)
  - [x] ~signature검증법 개선 (strncmp()말고 좀더 좋게...)~
  - [ ] Logger 고도화
    - [ ] Logger 옵션 설정
      - [ ] 시간대 설정
        - [ ] 기본값(옵션 안주었을 때) 서울 표준시 (가능하다면 ip를 이용해 위도 + 경도 흭득 후 해당 위치 표준시로 설정)
    - [ ] System Log 추가
      - [ ] agent 실행 시 로깅
        - [ ] 실행 시간, 실행 유저 정보, PID, PPID 등
      - [ ] agent 종료 시 로깅
        - [ ] Daemon이면 종료시킬 때 시그널을 보내서 할텐데, 종료 때 시그널이 무엇인지 조사하여 처리
          - [ ] Abort
          - [ ] Segfault
          - [ ] Bus Error
          - [ ] 종료 시간(종료 관련 시그널 받은 시간), 실행 유저 정보, PID, PPID, 간략한 메모리 정보, 간략한 CPU 정보 등
- [ ] Agent
  - [ ] Deamon으로 전환
  - [ ] 고도화
    - [x] ~Disk 정보 수집 및 전송~
  - [ ] 실행 옵션 고도화
    - [ ] 각 옵션마다 수집 주기 미입력 시 기본값 세팅 후 수집
    - [x] ~옵션의 수집 주기가 기본값보다 낮을 시 기본값으로 세팅 후 수집~
    - [ ] Log 저장 경로 옵션으로 입력. 미입력시 기본값 
  
- [ ] Server
  - [ ] UDP 통신 스레드 생성
  - [ ] 데이터 수집 시 특정 자료구조에 담아두고 Delta, Average 계산
    - [ ] CPU usage
      - [ ] CPU usage per process
      - [ ] CPU usage per seconds
    - [ ] Memory usage
      - [ ] Memory usage per seconds
    - [ ] Network throughput per seconds
      - [ ] number of packet
      - [ ] I/O bytes per seconds
  - [ ] Daemon 전환 준비
    - [x] ~서버가 동작 중일 때 agent는 언제든 다시 연결 가능하게 한다.~
    - [x] ~표준 출력이나 에러 출력으로 출력하면 안되고 Log를 남긴다.~
      - [x] ~자체제작 Log라이브러리를 통해 Logging~
- [ ] LD-PRELOAD
  - [x] ~학습 및 이해하기~
  - [ ] 구현하기
    - [x] ~송신하는 패킷 후킹하여 송신하기~
    - [ ] 서버에서 수신하여 별도의 테이블에 저장하기
# Checked todo list 
- [ ] 공통
  - [x] ~패킷 데이터 자료형 다시 생각해보기 (man 5 proc 참조(각 자료에 대한 자료형 명시되어있음))~
  - [x] ~동적 라이브러리로 분할하기~
    - [x] ~SMSutils - Logger, Queue, ...~
    - [x] ~collector - CPU, Memory, Network, Process, ...~
  - [x] ~Logger~
    - [x] ~서버와 클라이언트의 동작을 로깅하기 위한 로깅 라이브러리 제작~
    - [x] ~로깅은 저장소 설계 (파일에 text or DB)~
      - [x] ~log폴더 없을 시 생성하여 일자별로 로깅~
    - [x] ~로그 포맷 설계~
    - [x] ~printf에서 Log()로 전환~
      - [x] ~Sender~
      - [x] ~CPU Routine~
      - [x] ~Memory Routine~
      - [x] ~Network Routine~
      - [x] ~Process Routine~

- [ ] Agent
  - [x] ~데이터 수집하여 패킷으로 만들고 송신하기 (각 정보별로 스레드 동작)~
    - [x] ~CPU 정보~
    - [x] ~메모리 정보~
    - [x] ~네트워크 정보~
    - [x] ~프로세스 정보~
  - [x] ~실행 옵션 처리~
    - [x] ~각 옵션은 옵션 매개변수를 함께 사용해야함 (ex. ./agent -C 3000 -m 2500 -p 10000 -n 500)~
    - [x] ~-C 옵션: CPU 정보 수집~
    - [x] ~-m 옵션: memory 정보 수집~
    - [x] ~-p 옵션: 모든 프로세스 정보 수집~
    - [x] ~-n 옵션: 네트워크 정보수집~
    - [x] ~각 옵션에 대한 매개변수: 밀리초 단위로 표현하며 수집 주기를 의미~
  - [x] ~Daemon 전환 준비~
    - [x] ~연결이 끊겨도 프로세스가 죽으면 안된다.~
      - [x] ~연결이 끊겼을 때, 특정 횟수를 특정 주기로 재연결을 시도해야함~
      - [x] ~설정한 횟수만큼 재연결 시도를 했음에도 재연결이 되지 않으면 프로세스 종료~
    - [x] ~표준 출력이나 에러 출력으로 출력하면 안되고 Log를 남긴다.~
        - [x] ~자체제작 Log라이브러리를 통해 Logging~
  - [ ] 고도화
    - [x] ~변동없는 데이터는 최초에 한번만 서버에 송신하고, 그 이후에는 송신하지 않기~
  
  
- [x] ~Server~
  - [x] ~간단한 테스팅을 위한 TCP 서버 구현~
    - [x] ~싱글 스레드~
    - [x] ~연결 클라이언트 1개~
    - [x] ~패킷 수신 후 파싱하여 Agent 출력처럼 출력하기~
  - [x] ~약간 업그레이드~
    - [x] ~멀티 스레딩~
    - [x] ~연결 클라이언트는 agent의 스레드 수 만큼~
  - [x] ~멀티 프로세싱~
    - [x] ~각 커넥션 별로 프로세스 생성 후 패킷 수신~
  - [x] ~규격과 일치 하지 않은 패킷 받을 시 연결 종료~
    - [x] ~패킷에 적혀있는 정보를 이용하여 패킷 총 사이즈와 receive 사이즈를 비교~
    - [x] ~패킷에 적혀있는 시그니쳐 검사~ 
  - [x] ~DB 관련 처리~
    - [x] ~각종 수집 정보 저장~
      - [x] ~CPU 정보 저장~
      - [x] ~Memory 정보 저장~
      - [x] ~Network 정보 저장~
      - [x] ~Process 정보 저장~
      - [x] ~Disk 정보 저장~
    - [x] ~N개의 Worker 스레드에서 하나의 DB 커넥션에 문제없이 Query할 수 있도록 처리 (locking)~
  - [x] ~시스템 로깅 하기~
    - [x] ~server 시작 혹은 종료 혹은 에러 등~