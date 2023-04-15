# Desearch

Desearch is an experimental system towards a decentralized search engine.

To achieve good scalability and fault tolerance, desearch decouples computation and storage using a *stateless* trusted network and a *stateful*  cloud store regulated by blockchain.

The current desearch implementation uses Intel SGX as the trusted hardware and Redis as the store.

**Warning: This repo hosts an academic proof-of-concept prototype and has not received a careful code review.**

## Publication

Please refer to [our OSDI'21 paper](https://www.usenix.org/conference/osdi21/presentation/li) for more details.

~~~
@inproceedings{li2021desearch,
  author    = {Mingyu Li and Jinhao Zhu and Tianxu Zhang and Cheng Tan and Yubin Xia and Sebastian Angel and Haibo Chen},
  title     = {Bringing Decentralized Search to Decentralized Services},
  booktitle = {15th {USENIX} Symposium on Operating Systems Design and Implementation, {OSDI} 2021},
  pages     = {331--347},
  publisher = {{USENIX} Association},
  year      = {2021},
}
~~~

## Getting Started

Hardware Requirement: SGX-capable desktops or SGX-capable cloud machines. To check whether your machine supports SGX, please refer to [Intel SGX Hardware](https://github.com/ayeks/SGX-hardware).

Note that if you wish to run several SGX nodes on your local machine without support of scalable SGX, it might take longer to bootstrap the whole system because of the scarce encrypted memory (usually 128MB/256MB on SGXv1).

This repo provides a non-SGX version in the `executor` folder, and an SGX version in the `sgx-executor` folder. The non-SGX version helps you debug more easily if you want to build extensions to desearch; it uses the same folder structure as the SGX version. Note that only the SGX version contains ORAM-based queriers.

### Prerequisite

Under Ubuntu 20.04, building desearch executors requires the following depecencies installed:

- [SGX SDK + SGX PSW](https://01.org/intel-software-guard-extensions/downloads)
- [Boost C++ Libraries](https://www.boost.org/)
- [Hiredis](https://github.com/redis/hiredis)
- [Redis++](https://github.com/sewenew/redis-plus-plus)

You can refer to the `deps` folder for the correct versions of Redis, Hiredis, and Redis++.

```shell
apt install -y libboost-all-dev

cd deps
tar zxf redis-6.2.6.tar.gz
tar zxf redis-plus-plus-1.2.3.tar.gz

pushd redis-6.2.6/deps/hiredis/
make -j$(nproc) && make install
popd

pushd redis-plus-plus-1.2.3
mkdir build && cd build && cmake ..
make -j$(nproc) && make install
popd
```

### Kanban

Kanban is an unmodified [Redis](https://redis.io/) that leverages relatively cheap cloud storage.

Simply start the Redis server:
```shell
cd Kanban
cd deps && make hdr_histogram linenoise jemalloc
cd - && make all
./src/redis-server ./redis.conf
```

To clear all states from Kanban, you can issue `./src/redis-cli FLUSHALL`.

### Manager

Manager is a special executor that makes Kanban resistant to tampering.

```shell
cd manager
make all
./manager
```

### Executor

Executor consist of the whole search pipeline. This release consolidates all search roles within one executable. You are free to modify `executor/start.sh` to launch more executors as you like.

```shell
cd executor
make all
bash ./start.sh
```

### Client

Client is a Web server that serves as the entry for desearch.

```sh
cd client
make all
./client
```
Then double click the `client/index.html`, or use a fancy Web entry `client/WebUI/index.html`.

![demo](img/demo.png)
![fancy](img/fancy.png)

### How to build a distributed setup

To extend to a WAN setup, you need to modify a few network configurations:
- `config.hpp`: change `KANBAN_ADDR` to a global public IP
- `executor/start.sh`: change querier IP address to a public one that clients can reach

### Limitations and FAQs

See [executor/README.md](executor/README.md)

## Contributors

- Mingyu Li: maxullee [at] sjtu [dot] edu [dot] cn
- Jinhao Zhu: zhujinhao [at] sjtu [dot] edu [dot] cn
- Tianxu Zhang: zhangtianxu [at] sjtu [dot] edu [dot] cn
- Sajin Sasy: [ZeroTrace](https://github.com/sshsshy/ZeroTrace)

## License

MulanPSL-2.0 (see [here](https://opensource.org/licenses/MulanPSL-2.0))
