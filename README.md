# SMS
*S*ystem *M*onitoring *S*ystem with exam!

# Todo List

[] Agent
    [] ...
[] Server
    [] 간단한 TCP 서버 구현
        [] 싱글 스레드
        [] 연결 클라이언트 1개
        [] 패킷 수신 후 파싱하여 Agent 출력처럼 출력하기
    [] 본격적인 구현
        [] 각 연결마다 스레드 생성
            [] 스레드 풀 활용
        [] UDP 통신 스레드 생성
        [] DB 저장 구현   