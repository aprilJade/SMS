NAME		=	libhook_module.so
BIN_PATH	=	../../bin

$(NAME):
	cc -c sendHooker.c -o sendHooker.o
	cc -fPIC -shared -o $(BIN_PATH)/$(NAME) sendHooker.o -ldl 


all: $(NAME)

clean:
	rm -rf sendHooker.o

fclean: clean
	rm -rf $(BIN_PATH)/$(NAME)

re:
	make fclean
	make all

.PHONY: all clean fclean re d