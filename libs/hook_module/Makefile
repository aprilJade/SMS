NAME		=	libhook_module.so
SRC_DIR		=	./srcs
SRCS		=	$(notdir $(wildcard $(SRC_DIR)/*.c))

ifeq (, $(wildcard $(OBJ_DIR)))
$(shell mkdir $(OBJ_DIR))
endif

$(NAME): $(SRCS)
	cc -fPIC -shared -o $(NAME) $(OBJECTS) -ldl 


all: $(NAME)

clean:
	rm -rf $(OBJ_DIR)

fclean: clean
	rm -rf $(NAME)

re:
	make fclean
	make all

.PHONY: all clean fclean re d