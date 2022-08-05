# SMS
System Monitoring System의 약자입니다. 대상 OS를 모니터링하여 분석하는 시스템입니다.

# 실행방법

`주의! Ubuntu 22.04에서만 테스트 된 코드로 다른 환경에서 동작이 보장되지 않습니다.`

1. 해당 레포지토리를 클론해주세요.
 
`$ git clone https://github.com/aprilJade/SMS.git`

2. 빌드 후 서버를 실행해주세요.
```
cd SMS
make
./server/server
```
3. 이후 새로운 터미널을 켜고 agent를 실행해주세요.
```
./agent/agent
```
4. 반드시 agent부터 Ctrl + C를 입력하여 종료해주세요.

`빠른 시일 내에 상식적인 동작을 구현하겠습니다...!`

# Todo List
생각 나는 것을 위주로 적어가고 있습니다. 완전한 것은 아니고 계속 변경될 여지가 있습니다.
또한 추후 추가되거나 삭제될 리스트가 있을 수 있습니다.
- [ ] 공통
  - [ ] 코드 정리 (수시로 반복할 것)
  - [ ] 패킷 데이터 자료형 다시 생각해보기 (man 5 proc 참조(각 자료에 대한 자료형 명시되어있음))
  - [ ] 공통으로 쓰이는 함수 라이브러리로 제작
  - [ ] Logger
    - [ ] 서버와 클라이언트의 동작을 로깅하기 위한 로깅 라이브러리 제작
    - [ ] 로깅은 저장소 설계 (파일에 text or DB)
      - [ ] log폴더 없을 시 생성하여 일자별로 로깅
    - [ ] 로그 포맷 설계
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
  - [x] ~변동없는 데이터는 최초에 한번만 서버에 송신하고, 그 이후에는 송신하지 않기~
    - [x] ~CPU 정보 (Logical Core 개수)~
    - [x] ~메모리 정보 (메모리 총량 및 스왑 공간 총량)~
    - [x] ~네트워크 정보 (네트워크 인터페이스 종류와 개수)~
  - [ ] Daemon 으로써....
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
    - [ ] UDP 통신 스레드 생성
    - [ ] 각 연결마다 프로세스 생성
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
      - [ ] 서버가 동작 중일 때 agent는 언제든 다시 연결 가능하게 한다.
      - [ ] 표준 출력이나 에러 출력으로 출력하면 안되고 Log를 남긴다.
        - [ ] 자체제작 Log라이브러리를 통해 Logging
        - [ ] Server 시작할 때 Log를 남길지 말지 옵션 선택 가능하도록.