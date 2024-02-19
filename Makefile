CC ?= gcc
CXX ?= g++
CPP ?= g++

APP_NAME = tcp_i2c
APP_OBJ_FILES = tcp_i2c.o

LIBS = 

all: $(APP_NAME)

$(APP_NAME) : $(APP_OBJ_FILES)
	$(CXX) $^ -o $@  $(LIBS)

%.o : %.cc
	$(CXX) -c $^ -o $@


clean:
	rm *.o $(APP_NAME)