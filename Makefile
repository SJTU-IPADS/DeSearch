default: all

all:
	cd client ; make all -j8 ; cd -
	cd manager ; make all -j8 ; cd -
	cd executor ; make all -j8 ; cd -
	cd kanban/deps ; make hdr_histogram linenoise jemalloc -j8
	cd kanban ; make -j8
	cd kanban ; ./src/redis-server ./redis.conf &
	sleep 1
	cd kanban ; ./src/redis-cli FLUSHALL
	pkill redis-server

clean:
	cd client ; make clean ; cd -
	cd manager ; make clean ; cd -
	cd executor ; make clean ; cd -
	cd kanban ; make clean ; cd -
