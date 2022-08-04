TARGET:=$(notdir $(shell pwd))

all:$(TARGET) MakeSH

CXXFLAGS=-I../../depends/include
LDFLAGS=-L../../depends/lib
LIBS=-levent -lz

SRCS:=$(wildcard *.cpp)
OBJS:=$(patsubst %.cpp,%.o,$(SRCS))

#启动脚本
STARTSH=start_$(TARGET)

define MakeStartSH
	@echo "begin make start shell"
	@echo "LD_LIBRARY_PATH=../../depends/lib" > $(STARTSH)
	@echo "export LD_LIBRARY_PATH" >> $(STARTSH)
	@echo "./$(TARGET)" >> $(STARTSH)
	@chmod +x $(STARTSH)
	@echo "end make start shell"
endef

$(TARGET):$(OBJS)
	g++ $^ -o $@ -g $(LDFLAGS) $(LIBS)

MakeSH:
	$(call MakeStartSH)

clean:
	rm -rf $(TARGET) $(OBJS) $(STARTSH)