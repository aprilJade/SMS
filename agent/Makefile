NAME			=	agent
CFLAG			=	-g -fsanitize=address
INCLUDES		=	-I./includes/
SRC_DIR			=	./srcs
OBJ_DIR			=	./objs
SRCS	=	$(notdir $(wildcard $(SRC_DIR)/*.c))
OBJS	=	$(SRCS:.c=.o)
OBJECTS	=	$(patsubst %.o, $(OBJ_DIR)/%.o, $(OBJS))

ifdef DEBUG
	CFLAG += -g -fsanitize=address
endif

ifeq (, $(wildcard $(OBJ_DIR)))
$(shell mkdir $(OBJ_DIR))
endif

$(OBJ_DIR)/%.o	: $(SRC_DIR)/%.c
	$(CC) $(CFLAG) -c $< -o $@ $(INCLUDES) 

$(NAME): $(OBJECTS)
	cc $(CFLAG) $(INCLUDES) -o $(NAME) $(OBJECTS)

all: $(NAME)

clean:
	rm -rf $(OBJECTS)

fclean: clean
	rm -rf $(NAME)

re:
	make fclean
	make all

d:
	make fclean
	make DEBUG=1 all

.PHONY: all clean fclean re d