all:
	make -C ./libs
	make -C ./agent
	make -C ./server
	make -C ./agent-client
fclean:
	make fclean -C ./libs
	make fclean -C ./agent
	make fclean -C ./server
	make fclean -C ./agent-client
clean:
	make clean -C ./libs
	make clean -C ./agent
	make clean -C ./server
	make clean -C ./agent-client
re:
	make re -C ./libs
	make re -C ./agent
	make re -C ./server
	make re -C ./agent-client
lib:
	make -C ./libs
server:
	make -C ./libs
	make -C ./server
agent:
	make -C ./libs
	make -C ./agent
agent-cli:
	make fclean -C ./agent-client
	make -C ./agent-client

.PHONY: all fclean clean re server agent SMSutils collector