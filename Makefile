all:
	make -C ./libs
	make -C ./agent
	make -C ./server
fclean:
	make fclean -C ./libs
	make fclean -C ./agent
	make fclean -C ./server
clean:
	make clean -C ./libs
	make clean -C ./agent
	make clean -C ./server
re:
	make re -C ./libs
	make re -C ./agent
	make re -C ./server
lib:
	make -C ./libs
server:
	make -C ./libs
	make -C ./server
agent:
	make -C ./libs
	make -C ./agent

.PHONY: all fclean clean re server agent SMSutils collector