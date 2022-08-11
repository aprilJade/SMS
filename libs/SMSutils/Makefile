NAME		=	libSMSutils.so
INCLUDES	=	-I./includes/
LIB			=	-lpthread
SRC_DIR		=	./srcs
OBJ_DIR		=	./objs
SRCS		=	$(notdir $(wildcard $(SRC_DIR)/*.c))
OBJS		=	$(SRCS:.c=.o)
OBJECTS		=	$(patsubst %.o, $(OBJ_DIR)/%.o, $(OBJS))


ifeq (, $(wildcard $(OBJ_DIR)))
$(shell mkdir $(OBJ_DIR))
endif

$(OBJ_DIR)/%.o	: $(SRC_DIR)/%.c
	$(CC) -fPIC $(LIB) -c $< -o $@ $(INCLUDES) 

$(NAME): $(OBJECTS)
	cc -shared -o $(NAME) $(OBJECTS)

all: $(NAME)

clean:
	rm -rf $(OBJ_DIR)

fclean: clean
	rm -rf $(NAME)

re:
	make fclean
	make all

.PHONY: all clean fclean re