CC = clang++
CFLAGS = -Wall -Wextra -MMD -std=c++11 -Wshadow #-fsanitize=address
LDFLAGS = -levent
SRCS = $(shell find . -type f -name "*.cpp")

OBJ = $(SRCS:.cpp=.o)
DEPENDS = ${SRCS:.cpp=.d}
NAME = proxy

.PHONY: all clean fclean re run

all: $(SRCS) $(NAME)

$(NAME): $(OBJ)
		$(CC) $(LDFLAGS) $(OBJ) -o $(NAME)

.cpp.o: $(SRCS)
		$(CC) $(CFLAGS) -c $<

clean:
		@rm -rf $(OBJ) $(DEPENDS)

fclean: clean
		@rm -rf $(NAME)

re: fclean all

-include ${DEPENDS}
