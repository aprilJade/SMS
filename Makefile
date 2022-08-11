all:
	make -C ./libs/SMSutils
	make -C ./libs/collector
	make -C ./agent
	make -C ./server
fclean:
	make fclean -C ./libs/SMSutils
	make fclean -C ./libs/collector
	make fclean -C ./agent
	make fclean -C ./server
clean:
	make clean -C ./libs/SMSutils
	make clean -C ./libs/collector
	make clean -C ./agent
	make clean -C ./server
re:
	make re -C ./libs/SMSutils
	make re -C ./libs/collector
	make re -C ./agent
	make re -C ./server
server:
	make -C ./libs/SMSutils
	make -C ./server
agent:
	make -C ./libs/SMSutils
	make -C ./libs/collector
	make -C ./agent
SMSutils:
	make fclean -C ./libs/SMSutils
	make -C ./libs/SMSutils
collector:
	make fclean -C ./libs/collector
	make -C ./libs/collector
.PHONY: all fclean clean re server agent SMSutils collector