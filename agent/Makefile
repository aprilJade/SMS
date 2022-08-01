NAME			=	agent
CFLAG			=	-Wall -Wextra -Werror
INCLUDES		=	-I./includes/
SRC				=	main.c 
SRCS			=	$(addprefix ./srcs/, $(SRC))
OBJS			=	$(SRCS:.c=.o)

ifdef DEBUG
	CFLAG += -g -fsanitize=address
endif

%.o: %.c
	cc $(CFLAG) $(INCLUDES) $(BONUS_INCLUDES) -c $< -o $@ 

$(NAME): $(OBJS)
	cc $(CFLAG) $(INCLUDES) -o $(NAME) $(OBJS)

all: $(NAME)

clean:
	rm -rf $(OBJS) $(BONUS_OBJS)

fclean: clean
	rm -rf $(NAME) $(BONUS_NAME)

re:
	make fclean
	make all

d:
	make fclean
	make DEBUG=1 all

.PHONY: all clean fclean re d