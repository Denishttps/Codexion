NAME		= codexion
CC			= cc
CFLAGS		= -Wall -Wextra -Werror -pthread -Iincludes -MMD -MP
MAKEFLAGS	+= --no-print-directory

SRCS_DIR	= src
OBJS_DIR	= obj

# Colors
GREEN		= \033[1;32m
BLUE		= \033[1;34m
RED			= \033[1;31m
YELLOW		= \033[1;33m
RESET		= \033[0m

SRCS		= \
				main.c \
				parser.c \
				simulation.c \
				coder.c \
				monitor.c \
				utils.c \
				priority_queue.c

OBJS		= $(addprefix $(OBJS_DIR)/,$(SRCS:.c=.o))
DEPS		= $(OBJS:.o=.d)

all: $(NAME)

$(NAME): $(OBJS)
	@printf "  $(YELLOW)LD$(RESET)  %s\n" $@
	@$(CC) $(OBJS) -o $(NAME) -pthread
	@printf "$(GREEN)Build successful!$(RESET)\n"

$(OBJS_DIR)/%.o: $(SRCS_DIR)/%.c
	@mkdir -p $(dir $@)
	@printf "  $(BLUE)CC$(RESET)  %s\n" $<
	@$(CC) $(CFLAGS) -c $< -o $@

clean:
	@printf "  $(RED)RM$(RESET)  %s\n" $(OBJS_DIR)
	@rm -rf $(OBJS_DIR)

fclean: clean
	@printf "  $(RED)RM$(RESET)  %s\n" $(NAME)
	@rm -f $(NAME)

re: fclean all

-include $(DEPS)

.PHONY: all clean fclean re
