NAME		=	agent
BIN_PATH	=	../bin
INCLUDES	=	-I./includes/ -I../libs/SMSutils/includes -I../libs/collector/includes
LIB			=	-lpthread -lSMSutils -lcollector
LIB_DIR		=	-L../bin
SRC_DIR		=	./srcs
OBJ_DIR		=	./objs
SRCS		=	$(notdir $(wildcard $(SRC_DIR)/*.c))
OBJS		=	$(SRCS:.c=.o)
OBJECTS		=	$(patsubst %.o, $(OBJ_DIR)/%.o, $(OBJS))

ifeq (, $(wildcard $(OBJ_DIR)))
$(shell mkdir $(OBJ_DIR))
endif

ifeq (, $(wildcard $(BIN_PATH)))
$(shell mkdir $(BIN_PATH))
endif

$(OBJ_DIR)/%.o	: $(SRC_DIR)/%.c
	cc -c $< -o $@ $(INCLUDES) $(LIB_DIR) $(LIB)

$(NAME): $(OBJECTS)
	cc $(INCLUDES) -o ../bin/$(NAME) $(OBJECTS) $(LIB_DIR) $(LIB)

all: $(NAME)

clean:
	rm -rf $(OBJ_DIR)

fclean: clean
	rm -rf $(BIN_PATH)/$(NAME)

re:
	make fclean
	make all

.PHONY: all clean fclean re