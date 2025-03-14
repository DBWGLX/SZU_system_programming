CXX = g++
CXXFLAGS = -std=c++17 -pthread
INCLUDES = -I/usr/include/mysql-cppconn/jdbc
LIBS = -lmysqlcppconn -ljansson -L /root/work/MyLogger/cmake_out/lib -lasync_logger -lcrypto

# 源文件
SRCS = chatserver_wyx.cpp DBOperation.cpp EpollServer.cpp MySQLConnectionPool.cpp ThreadPool.cpp User.cpp LRUTokenManager.cpp

# 目标文件
TARGET = chatserver_wyx

# 规则
all: $(TARGET)

$(TARGET): $(SRCS)
	$(CXX) $^ $(CXXFLAGS) $(INCLUDES) $(LIBS) -o $@

# 清理编译生成的文件
.PHONY: clean
clean:
	rm -rf $(TARGET)
