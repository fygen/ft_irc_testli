
# Simple ft_irc Makefile (C++98)
NAME := ircserv
CXX := c++
CXXFLAGS := -g -Wall -Wextra -Werror -std=c++98
INCLUDES := -Iinclude

SRC := \
	src/main.cpp \
	src/Server.cpp \
	src/Server_handleLine.cpp \
	src/Client.cpp \
	src/Channel.cpp \
	src/Parser.cpp \
	src/Commands.cpp \
	src/Utils.cpp \
	src/Replies.cpp

OBJ := $(SRC:.cpp=.o)

all: $(NAME)

$(NAME): $(OBJ)
	$(CXX) $(CXXFLAGS) $(INCLUDES) -o $@ $^

%.o: %.cpp
	$(CXX) $(CXXFLAGS) $(INCLUDES) -c $< -o $@

clean:
	rm -f $(OBJ)
	rm -rf ./tests/output

fclean: clean
	rm -f $(NAME)

re: fclean all

test: clean all
	@echo "Running tests..."
	# Add your test commands here
	./tests/test_run2.sh 4444 4444

.PHONY: all clean fclean re test
