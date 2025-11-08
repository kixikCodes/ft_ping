#					COMPILATION INFORMATION
CC = cc
FLAGS = -Wall -Wextra -Werror -Iinclude #-g -fsanitize=address
NAME = ft_ping
GREEN = \033[0;32m
RESET = \033[0m

#					SOURCE AND OBJECT FILES
LIB = libft/libft.a
SRC_DIR = src
SRC_FILES = $(SRC_DIR)/ft_ping.c $(SRC_DIR)/icmp.c $(SRC_DIR)/utils.c $(SRC_DIR)/error.c
OBJ = $(SRC_FILES:.c=.o)

#					CODE AND RULES
.NOTPARALLEL:
.FORCE:

all: $(LIB) $(NAME)

$(LIB):
	$(MAKE) -C libft

$(NAME): $(OBJ) $(LIB)
	@$(CC) $(FLAGS) -o $(NAME) $(OBJ) $(LIB)
	@printf "$(GREEN)Successfully compiled: $(RESET)%s\n" $(NAME)

$(SRC_DIR)/%.o: $(SRC_DIR)/%.c
	@mkdir -p $(dir $@)
	@printf "$(GREEN)Compiling: $(RESET)%s\n" $(notdir $<)
	@$(CC) $(FLAGS) -c $< -o $@

clean:
	@rm -f $(OBJ)
	@$(MAKE) -C libft clean
	@rm libft/libft.a
	@printf "$(GREEN)Cleaned object files$(RESET)\n"

fclean: clean
	@rm -f $(NAME)
	@printf "$(GREEN)Cleaned everything$(RESET)\n"

re: fclean all

reclean: re .FORCE
	$(MAKE) clean

allclean: all .FORCE
	$(MAKE) clean

.PHONY: all clean fclean re reclean allclean .FORCE
