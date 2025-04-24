NAME		:=	webserv
RM			:=	rm -rf
CPP			:=	c++
CPPFLAGS	:=	-Wall -Wextra -Werror -std=c++17
DEBUG 		?= 0
ifeq ($(DEBUG), 1)
CPP			:= g++
CPPFLAGS	:= -g -std=c++17
endif

SRCDIR		:=	src
SRC			:= $(SRCDIR)/main.cpp \
				$(SRCDIR)/Webserv/Epoll.cpp \
				$(SRCDIR)/Webserv/SharedFd.cpp \
				$(SRCDIR)/Webserv/Webserv.cpp \
				$(SRCDIR)/HTTP/HTTPClient.cpp \
				$(SRCDIR)/HTTP/HTTPParser.cpp \
				$(SRCDIR)/HTTP/HTTPResponse.cpp \
				$(SRCDIR)/HTTP/ClientCommunication.cpp \
				$(SRCDIR)/Config/Config.cpp \
				$(SRCDIR)/Config/ConfigParser.cpp \
				$(SRCDIR)/CGI/CGIPipes.cpp \
				$(SRCDIR)/CGI/CGIResponse.cpp \
				$(SRCDIR)/CGI/CGISendData.cpp \
				$(SRCDIR)/CGI/CGIStart.cpp \
				$(SRCDIR)/CGI/CGI.cpp

INCL        :=   -I./inc

OBJDIR		:=	.build
OBJ			:=	$(SRC:$(SRCDIR)/%.cpp=$(OBJDIR)/%.o)

all:		$(NAME)

$(NAME):	$(OBJ)
			@$(CPP) $(OBJ) -o $(NAME) $(INCL)
			@printf "$(CREATED)" $@ $(CUR_DIR)

$(OBJDIR)/%.o:	$(SRCDIR)/%.cpp
			@mkdir -p $(@D)
			@$(CPP) $(CPPFLAGS) -c $< -o $@
			@printf "$(UPDATED)" $@ $(CUR_DIR)

clean:
			$(RM) $(OBJDIR)
			@printf "$(REMOVED)" $(OBJDIR) $(CUR_DIR)

fclean: clean
			$(RM) $(NAME) $(NAME_DEBUG)
			@printf "$(REMOVED)" $(NAME) $(CUR_DIR)

re:			fclean all

info-%:
	$(info $($*))

.PHONY: all clean fclean re
.SILENT:

# ----------------------------------- colors --------------------------------- #

BOLD	:= \033[1m
BLACK	:= \033[30;1m
RED		:= \033[31;1m
GREEN	:= \033[32;1m
YELLOW	:= \033[33;1m
BLUE	:= \033[34;1m
MAGENTA	:= \033[35;1m
CYAN	:= \033[36;1m
WHITE	:= \033[37;1m
RESET	:= \033[0m

# ----------------------------------- messages ------------------------------- #

CUR_DIR := $(dir $(abspath $(firstword $(MAKEFILE_LIST))))
REMOVED	:= \t$(RED)$(BOLD)REMOVED %s (%s) $(RESET)\n
MADE	:= \t$(GREEN)$(BOLD)MAKE -C %s (%s) $(RESET)\n
CREATED	:= \t$(GREEN)$(BOLD)CREATED %s (%s) $(RESET)\n
UPDATED	:= \t$(BLUE)$(BOLD)CREATED OR UPDATED %s (%s) $(RESET)\n
