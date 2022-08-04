# SMS
System Monitoring System의 약자입니다. 대상 OS를 모니터링하여 분석하는 시스템입니다.

# 실행방법
1. 해당 레포지토리를 클론해주세요.
`$ git clone https://github.com/aprilJade/SMS.git`
2. 빌드 후 server 실행 후 agent를 실행해주세요.
```
$ cd SMS
$ make
$ ./server/server
$ ./agent/agent
```

# Todo List
생각 나는 것을 위주로 적어가고 있습니다. 완전한 것은 아니고 계속 변경될 여지가 있습니다.
또한 추후 추가되거나 삭제될 리스트가 있을 수 있습니다.
- [ ] 공통
    - [ ] 공통으로 쓰이는 함수 라이브러리로 제작
    - [ ] Logger
        - [ ] 서버와 클라이언트의 동작을 로깅하기 위한 로깅 라이브러리 제작
        - [ ] 로깅은 파일로 저장
            - [ ] log폴더 없을 시 생성하여 일자별로 로깅
- [ ] Agent
    - [ ] 데이터 수집하여 패킷으로 만들고 송신하기 (각 정보별로 스레드 동작)
        - [x] ~CPU 정보~
        - [ ] 메모리 정보
        - [ ] 네트워크 정보
        - [ ] 프로세스 정보
    - [ ] 데이터 수집 시 특정 자료구조에 담아두고 Delta, Average 계산
        - [ ] CPU usage
        - [ ] CPU usage per process
        - [ ] Network bytes per
    - [ ] 설정 파일 제작 후 수집 주기, 송신 대상 서버 정보 등 설정기능 구현
        - [ ] 후보군: ini, json 혹은 custom.
- [ ] Server
    - [x] ~간단한 테스팅을 위한 TCP 서버 구현~
        - [x] ~싱글 스레드~
        - [x] ~연결 클라이언트 1개~
        - [x] ~패킷 수신 후 파싱하여 Agent 출력처럼 출력하기~
    - [ ] 본격적인(?) 구현
        - [ ] 각 연결마다 스레드 생성
            - [ ] 스레드 풀 활용
        - [ ] UDP 통신 스레드 생성
        - [ ] DB 저장 구현   