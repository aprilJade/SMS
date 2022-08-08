all:
	make -C ./SMSutils
	make -C ./agent
	make -C ./server
fclean:
	make fclean -C ./SMSutils
	make fclean -C ./agent
	make fclean -C ./server
clean:
	make clean -C ./SMSutils
	make clean -C ./agent
	make clean -C ./server
re:
	make re -C ./SMSutils
	make re -C ./agent
	make re -C ./server
server:
	make -C ./SMSutils
	make -C ./server
	make fclean -C ./server
agent:
	make -C ./SMSutils
	make -C ./agent
	make fclean -C ./agent
lib:
	make fclean -C ./SMSutils
	make -C ./SMSutils

.PHONY: all fclean clean re server agent lib