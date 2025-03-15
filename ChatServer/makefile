CXX = g++
CXXFLAGS = -std=c++17 -pthread
INCLUDES = -I/usr/include/mysql-cppconn/jdbc
LIBS = -lmysqlcppconn -ljansson -L /root/work/MyLogger/cmake_out/lib -lasync_logger -lcrypto

# 源文件和目标文件
SRCS = chatserver_wyx.cpp DBOperation.cpp EpollServer.cpp MySQLConnectionPool.cpp ThreadPool.cpp User.cpp LRUTokenManager.cpp ClientTask.cpp
OBJS = $(SRCS:.cpp=.o)
TARGET = chatserver_wyx

# 默认目标
all: $(TARGET)

# 链接阶段，只在 .o 文件变化时才重新链接
$(TARGET): $(OBJS)
	$(CXX) $(CXXFLAGS) $(INCLUDES) $^ $(LIBS) -o $@

# 编译阶段，只在单个源文件变化时才重新编译
%.o: %.cpp
	$(CXX) $(CXXFLAGS) $(INCLUDES) -c $< -o $@

# 清理编译生成的文件
.PHONY: clean
clean:
	rm -rf $(OBJS) $(TARGET)
