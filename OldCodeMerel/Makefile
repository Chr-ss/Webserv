CC			= c++
NAME		= webserv
CFLAGS		= -std=c++20 -Wall -Wextra -Werror -Iincludes -g -g3
SRCFILES	= main.cpp Webserv.cpp Server.cpp Epoll.cpp Client.cpp CGI.cpp
HEADERFILES	= Webserv.hpp Server.hpp Client.hpp CGI.hpp

DIR_SRC		= src/
DIR_OBJ		= obj/
DIR_INC		= includes/

ALL_INC		= $(addprefix $(DIR_INC), $(HEADERFILES))
ALL_OBJ		= $(addprefix $(DIR_OBJ), $(SRCFILES:.cpp=.o))
ALL_SRC		= $(addprefix $(DIR_SRC), $(SRCFILES))

#----------------------------------------------------------------------------

all: $(NAME)

$(NAME): $(ALL_OBJ)
	$(CC) $(CFLAGS) $(ALL_OBJ) -o $(NAME)

re: fclean all

clean:
	rm -rf $(DIR_OBJ)
	
fclean:
	rm -rf $(DIR_OBJ) $(NAME)

$(DIR_OBJ)%.o: $(DIR_SRC)%.cpp $(ALL_INC)
	@mkdir -p $(DIR_OBJ)
	$(CC) -c $(CFLAGS) -o $@ $<

fsan: $(ALL_OBJ)
	$(CC) $(CFLAGS) $(ALL_OBJ) -fsanitize=address -o $(NAME)

run: $(NAME)
	./$(NAME)

valg:
	valgrind ./$(NAME)

.PHONY: clean fclean re all
