# SMS
System Monitoring System의 약자입니다. 대상 OS를 모니터링하여 분석하는 시스템입니다.

# 실행방법
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
    - [ ] 로깅은 파일로 저장
      - [ ] log폴더 없을 시 생성하여 일자별로 로깅
- [ ] Agent
  - [x] 데이터 수집하여 패킷으로 만들고 송신하기 (각 정보별로 스레드 동작)
    - [x] ~CPU 정보~
    - [x] ~메모리 정보~
    - [x] ~네트워크 정보~
    - [x] ~프로세스 정보~
  - [ ] 추후 구현
    - [ ] Disk 정보
    - [ ] 각 Logical Core별 정보 (running time, idle time, wait time, usage)
    - [ ] 지금은 NIC가 하나인 경우만 체크하나 복수의 네트워크 인터페이스의 정보를 수집하도록 변경
    - [ ] Network bytes per seconds
  - [ ] 설정 파일 제작 후 수집 주기, 송신 대상 서버 정보 등 설정기능 구현
    - [ ] 후보군: ini, json 혹은 custom.
  - [x] ~변동없는 데이터는 최초에 한번만 서버에 송신하고, 그 이후에는 송신하지 않기~
    - [x] ~CPU 정보 (Logical Core 개수)~
    - [x] ~메모리 정보 (메모리 총량 및 스왑 공간 총량)~
    - [x] ~네트워크 정보 (네트워크 인터페이스 종류와 개수)~
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