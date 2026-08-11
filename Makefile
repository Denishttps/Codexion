NAME = codexion

CC = cc
CFLAGS = -Wall -Wextra -Werror -pthread -Iincludes
SRCS_DIR = src

SRCS = main.c parser.c simulation.c coder.c monitor.c utils.c priority_queue.c
SRCS := $(addprefix $(SRCS_DIR)/, $(SRCS))
OBJS = $(SRCS:.c=.o)

all: $(NAME)

$(NAME): $(OBJS)
	$(CC) $(CFLAGS) $(OBJS) -o $(NAME)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(OBJS)

fclean: clean
	rm -f $(NAME)

re: fclean all

.PHONY: all clean fclean re
