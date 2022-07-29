TARGET:=$(notdir $(shell pwd))
SRCS:=$(wildcard *.cpp)
OBJS:=$(patsubst %.cpp,%.o,$(SRCS)) 
$(TARGET):$(OBJS)
	g++ $^ -o $@ -levent
clean:
	rm -rf $(TARGET) $(OBJS)