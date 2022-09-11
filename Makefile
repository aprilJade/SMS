all:
	make -C ./libs/SMSutils
	make -C ./libs/collector
	make -C ./libs/hook_module
	make -C ./agent
	make -C ./server
	make -C ./agent-client
fclean:
	make fclean -C ./libs/SMSutils
	make fclean -C ./libs/collector
	make fclean -C ./libs/hook_module
	make fclean -C ./agent
	make fclean -C ./server
	make fclean -C ./agent-client
clean:
	make clean -C ./libs/SMSutils
	make clean -C ./libs/collector
	make clean -C ./libs/hook_module
	make clean -C ./agent
	make clean -C ./server
	make clean -C ./agent-client
re:
	make re -C ./libs/SMSutils
	make re -C ./libs/collector
	make re -C ./libs/hook_module
	make re -C ./agent
	make re -C ./server
	make re -C ./agent-client
server:
	make -C ./libs/SMSutils
	make -C ./server
agent:
	make -C ./libs/SMSutils
	make -C ./libs/collector
	make -C ./libs/hook_module
	make -C ./agent
SMSutils:
	make fclean -C ./libs/SMSutils
	make -C ./libs/SMSutils
collector:
	make fclean -C ./libs/collector
	make -C ./libs/collector
hook_module:
	make fclean -C ./libs/hook_module
	make -C ./libs/hook_module
agent-cli:
	make fclean -C ./agent-client
	make -C ./agent-client
.PHONY: all fclean clean re server agent SMSutils collector