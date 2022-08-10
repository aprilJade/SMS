all:
	make -C ./SMSutils
	make -C ./collector
	make -C ./agent
	make -C ./server
fclean:
	make fclean -C ./SMSutils
	make fclean -C ./collector
	make fclean -C ./agent
	make fclean -C ./server
clean:
	make clean -C ./SMSutils
	make clean -C ./collector
	make clean -C ./agent
	make clean -C ./server
re:
	make re -C ./SMSutils
	make re -C ./collector
	make re -C ./agent
	make re -C ./server
server:
	make -C ./SMSutils
	make -C ./server
	make fclean -C ./server
agent:
	make -C ./SMSutils
	make -C ./collector
	make -C ./agent
SMSutils:
	make fclean -C ./SMSutils
	make -C ./SMSutils
collector:
	make fclean -C ./collector
	make -C ./collector
.PHONY: all fclean clean re server agent SMSutils collector