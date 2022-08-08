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
./server/server
```
1. 이후 새로운 터미널을 켜고 agent를 실행해주세요. 스크립트를 직접 수정하여 원하는 옵션을 입력해보세요.

`현재 소스코드 기준으로 6초간격으로 최대 100회 서버와 연결을 시도합니다. 그전에 서버를 실행해주세요.`

```
sh run-agent.sh
```


# Todo List
생각 나는 것을 위주로 적어가고 있습니다. 완전한 것은 아니고 계속 변경될 여지가 있습니다.
또한 추후 추가되거나 삭제될 리스트가 있을 수 있습니다.
- [ ] 공통
  - [ ] 코드 정리 (수시로 반복할 것)
  - [x] ~패킷 데이터 자료형 다시 생각해보기 (man 5 proc 참조(각 자료에 대한 자료형 명시되어있음))~
  - [x] ~동적 라이브러리로 분할하기~
    - [x] ~SMSutils - Logger, Queue, ...~
    - [x] ~collector - CPU, Memory, Network, Process, ...~
  - [ ] Logger
    - [ ] 서버와 클라이언트의 동작을 로깅하기 위한 로깅 라이브러리 제작
    - [x] ~로깅은 저장소 설계 (파일에 text or DB)~
      - [x] ~log폴더 없을 시 생성하여 일자별로 로깅~
    - [x] ~로그 포맷 설계~
    - [x] ~printf에서 Log()로 전환~
      - [x] ~Sender~
      - [x] ~CPU Routine~
      - [x] ~Memory Routine~
      - [x] ~Network Routine~
      - [x] ~Process Routine~
    - [ ] System Log 추가
      - [ ] malloc error, file open error, file read error,...
      - [ ] agent 실행 시 로깅
        - [ ] 실행 시간, 실행 유저 정보, PID, PPID 등
      - [ ] agent 종료 시 로깅
        - [ ] Daemon이면 종료시킬 때 시그널을 보내서 할텐데, 종료 때 시그널이 무엇인지 조사하여 처리
          - [ ] Abort
          - [ ] Segfault
          - [ ] Bus Error
          - [ ] 종료 시간(종료 관련 시그널 받은 시간), 실행 유저 정보, PID, PPID, 간략한 메모리 정보, 간략한 CPU 정보 등
- [ ] Agent
  - [x] ~데이터 수집하여 패킷으로 만들고 송신하기 (각 정보별로 스레드 동작)~
    - [x] ~CPU 정보~
    - [x] ~메모리 정보~
    - [x] ~네트워크 정보~
    - [x] ~프로세스 정보~
  - [ ] 추후 구현
    - [ ] Disk 정보
    - [ ] 각 Logical Core별 정보 (running time, idle time, wait time, usage)
    - [ ] 지금은 NIC가 하나인 경우만 체크하나 복수의 네트워크 인터페이스의 정보를 수집하도록 변경
    - [ ] Network bytes per seconds
  - [x] ~실행 옵션 처리~
    - [x] ~각 옵션은 옵션 매개변수를 함께 사용해야함 (ex. ./agent -C 3000 -m 2500 -p 10000 -n 500)~
    - [x] ~-C 옵션: CPU 정보 수집~
    - [x] ~-m 옵션: memory 정보 수집~
    - [x] ~-p 옵션: 모든 프로세스 정보 수집~
    - [x] ~-n 옵션: 네트워크 정보수집~
    - [x] ~각 옵션에 대한 매개변수: 밀리초 단위로 표현하며 수집 주기를 의미~
  - [ ] 실행 옵션 고도화
    - [ ] 각 옵션마다 수집 주기 미입력 시 기본값 세팅 후 수집
    - [x] ~옵션의 수집 주기가 기본값보다 낮을 시 기본값으로 세팅 후 수집~
  - [x] ~변동없는 데이터는 최초에 한번만 서버에 송신하고, 그 이후에는 송신하지 않기~
    - [x] ~CPU 정보 (Logical Core 개수)~
    - [x] ~메모리 정보 (메모리 총량 및 스왑 공간 총량)~
    - [x] ~네트워크 정보 (네트워크 인터페이스 종류와 개수)~
  - [ ] Daemon 으로써....
    - [x] ~연결이 끊겨도 프로세스가 죽으면 안된다.~
      - [x] ~연결이 끊겼을 때, 특정 횟수를 특정 주기로 재연결을 시도해야함~
      - [x] ~설정한 횟수만큼 재연결 시도를 했음에도 재연결이 되지 않으면 프로세스 종료~
    - [ ] 표준 출력이나 에러 출력으로 출력하면 안되고 Log를 남긴다.
        - [ ] 자체제작 Log라이브러리를 통해 Logging
        - [ ] agent 실행 시 Log 옵션 설정
          - [ ] 시간대 설정
            - [ ] 기본값(옵션 안주었을 때) 서울 표준시 (가능하다면 ip를 이용해 위도 + 경도 흭득 후 해당 위치 표준시로 설정)
          - [ ] Detail or Simple 선택 가능하도록 설정
            - [ ] 기본값(옵션 안주었을 때) Simple
            - [ ] Simple => 로깅 시간, 간단한 로그 내용 (어떤 함수가 호출 되었는가) (추가적인 고민 필요)
            - [ ] Detail => 로깅 시간, 자세한 로그 내용 (각 스레드별 TCP 송신 누적 패킷 수, 각 스레드별 구동 시간 등) (추가적인 고민 필요)
          - [ ] 파일 or DB 선택 가능하도록 설정
            - [ ] 기본값(옵션 안주었을 때) 파일로 저장
            - [ ] DB 설정 시 DBMS에 저장. (Redis 유력)
- [ ] Server
  - [x] ~간단한 테스팅을 위한 TCP 서버 구현~
    - [x] ~싱글 스레드~
    - [x] ~연결 클라이언트 1개~
    - [x] ~패킷 수신 후 파싱하여 Agent 출력처럼 출력하기~
  - [x] ~약간 업그레이드~
    - [x] ~멀티 스레딩~
    - [x] ~연결 클라이언트는 agent의 스레드 수 만큼~
  - [x] ~멀티 프로세싱~
    - [x] ~각 커넥션 별로 프로세스 생성 후 패킷 수신~
  - [ ] 본격적인(?) 구현
    - [x] ~규격과 일치 하지 않은 패킷 받을 시 연결 종료~
      - [x] ~패킷에 적혀있는 정보를 이용하여 패킷 총 사이즈와 receive 사이즈를 비교~
      - [x] ~패킷에 적혀있는 시그니쳐 검사~ 
    - [ ] UDP 통신 스레드 생성
    - [ ] 각 연결마다 스레드 생성
      - [ ] DB 저장 구현   
    - [ ] 데이터 수집 시 특정 자료구조에 담아두고 Delta, Average 계산
      - [ ] CPU usage
        - [ ] CPU usage per process
        - [ ] CPU usage per seconds
      - [ ] Memory usage
        - [ ] Memory usage per seconds
      - [ ] Network throughput per seconds
        - [ ] number of packet
        - [ ] I/O bytes per seconds
    - [ ] Daemon으로써...
      - [x] ~서버가 동작 중일 때 agent는 언제든 다시 연결 가능하게 한다.~
      - [ ] 표준 출력이나 에러 출력으로 출력하면 안되고 Log를 남긴다.
        - [ ] 자체제작 Log라이브러리를 통해 Logging
        - [ ] Server 시작할 때 Log를 남길지 말지 옵션 선택 가능하도록.