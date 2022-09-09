NAME		=	printer
INCLUDES	=	-I./includes/ \
				-I/usr/include/postgresql
LIB			=	-lpthread -lpq
LIB_DIR		=	-L../bin -L/usr/lib/postgresql/14/lib
SRCS		=	main.c
OBJS		=	$(SRCS:.c=.o)

%.o	: %.c
	cc -c main.c -o main.o $(INCLUDES) $(LIB_DIR) $(LIB) 

$(NAME): $(OBJS)
	cc $(INCLUDES) -o $(NAME) $(OBJS) $(LIB_DIR) $(LIB)

all: $(NAME)

clean:
	rm -rf main.o

fclean: clean
	rm -rf $(NAME)

re:
	make fclean
	make all

.PHONY: all clean fclean re