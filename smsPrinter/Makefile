NAME		=	printer
INCLUDES	=	-I./includes/ \
				-I/usr/include/postgresql
LIB			=	-lpthread -lpq
LIB_DIR		=	-L../bin -L/usr/lib/postgresql/14/lib
SRC_DIR		=	./srcs
OBJ_DIR		=	./objs
SRCS		=	$(notdir $(wildcard $(SRC_DIR)/*.c))
OBJS		=	$(SRCS:.c=.o)
OBJECTS		=	$(patsubst %.o, $(OBJ_DIR)/%.o, $(OBJS))

ifeq (, $(wildcard $(OBJ_DIR)))
$(shell mkdir $(OBJ_DIR))
endif

$(OBJ_DIR)/%.o	: $(SRC_DIR)/%.c
	cc -c $< -o $@ $(INCLUDES) $(LIB_DIR) $(LIB) 

$(NAME): $(OBJECTS)
	cc $(INCLUDES) -o $(NAME) $(OBJECTS) $(LIB_DIR) $(LIB)

all: $(NAME)

clean:
	rm -rf $(OBJ_DIR)

fclean: clean
	rm -rf $(NAME)

re:
	make fclean
	make all

.PHONY: all clean fclean re