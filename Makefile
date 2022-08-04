all:
	make -C ./agent
	make -C ./server
fclean:
	make fclean -C ./agent
	make fclean -C ./server
clean:
	make clean -C ./agent
	make clean -C ./server
re:
	make re -C ./agent
	make re -C ./server
server:
	make -C ./server
agent:
	make -C ./agent
.PHONY: all fclean clean re server agent