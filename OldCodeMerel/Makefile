CC			= c++
NAME		= webserv
CFLAGS		= -std=c++20 -I$(DIR_INC) -g -g3

DIR_SRC		= src/
DIR_OBJ		= obj/
DIR_INC		= includes/

SRCFILES	= $(wildcard $(DIR_SRC)*.cpp)
HEADERFILES	= $(wildcard $(DIR_SRC)*.hpp)

ALL_OBJ     = $(patsubst $(DIR_SRC)%.cpp, $(DIR_OBJ)%.o, $(SRCFILES))

#----------------------------------------------------------------------------

all: $(NAME)

$(NAME): $(ALL_OBJ)
	$(CC) $(CFLAGS) $(ALL_OBJ) -o $(NAME)

re: fclean all

clean:
	rm -rf $(DIR_OBJ)
	
fclean: clean
	rm -rf $(NAME)

$(DIR_OBJ)%.o: $(DIR_SRC)%.cpp
	@mkdir -p $(DIR_OBJ)
	$(CC) -c $(CFLAGS) -o $@ $<

fsan: $(ALL_OBJ)
	$(CC) $(CFLAGS) $(ALL_OBJ) -fsanitize=address -o $(NAME)

run: $(NAME)
	./$(NAME)

valg:
	valgrind ./$(NAME)

.PHONY: clean fclean re all
