# 编译器
CXX = g++
CXXFLAGS = -std=c++17 -pthread -Wall

# 目录
SRC_DIR = src
BIN_DIR = bin
OBJ_DIR = obj
# VPATH : 额外查找路径
VPATH = $(SRC_DIR)


INC_DIRS := $(shell find include -type d)
CXXFLAGS += $(addprefix -I,$(INC_DIRS))
# 头文件路径
INCLUDES = -I/usr/include/mysql-cppconn/jdbc

# 库
LIBS = -lmysqlcppconn -ljansson -L /root/work/MyLogger/cmake_out/lib -lasync_logger -lcrypto

# 递归查找 src 目录下的所有 .cpp 文件
SRCS := $(shell find $(SRC_DIR) -type f -name "*.cpp") 
OBJS := $(patsubst $(SRC_DIR)/%.cpp, $(OBJ_DIR)/%.o, $(SRCS))

TARGET = $(BIN_DIR)/chatserver_wyx

# 目标
all: $(TARGET)

# 链接目标程序
$(TARGET): $(OBJS)
	@mkdir -p $(BIN_DIR)
	$(CXX) $(CXXFLAGS) $(INCLUDES) $^ $(LIBS) -o $@
	@echo "Linking completed: $@"

# 编译 C++ 源文件
$(OBJ_DIR)/%.o: %.cpp
	@mkdir -p $(dir $@)  
	$(CXX) $(CXXFLAGS) $(INCLUDES) -c $< -o $@
	@echo "Compiled: $<"

# 清理编译生成的文件
.PHONY: clean
clean:
	rm -rf $(OBJ_DIR) $(BIN_DIR)
	@echo "Cleaned up object and binary files."