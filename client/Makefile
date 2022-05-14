CLIENT_Files := $(wildcard *.cpp) $(wildcard contrib/ed25519/*.cpp)
CLIENT_Objects := $(CLIENT_Files:.cpp=.o)

CC_Flags := -g -O0 -std=c++11 -w # -Wall
CC_Flags += -I.. -I./contrib/SimpleWeb -I./contrib/ed25519
CC_Flags += -I redis++/include
#CC_Flags += -I./contrib/hiredis/include -I./contrib/redis++/include -I/usr/local/Cellar/openssl/1.0.2s/include

Link_Flags := -L. -lpthread -ldl -lz
Link_Flags += -lcrypto -lredis++ -lhiredis
Link_Flags += -lboost_system -lboost_filesystem -lboost_serialization -lboost_iostreams
#Link_Flags += contrib/hiredis/lib/libhiredis.a contrib/redis++/lib/libredis++.a
#Link_Flags += /usr/local/Cellar/openssl/1.0.2s/lib/libcrypto.a


Target := client

.PHONY: all

%.o: %.cpp
	@$(CXX) $(CC_Flags) -c $< -o $@
	@echo "CC   <=  $<"

$(Target): $(CLIENT_Objects)
	@# rm logging
	@$(CXX) $^ -o $@ $(Link_Flags)
	@echo "LINK =>  $@"

all: $(Target)
	echo "Done"

clean:
	find . -name "*.o" | xargs rm
	rm $(Target)
