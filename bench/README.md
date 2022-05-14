# Benchmark

This folder hosts a candidate keyword list named `top-10k-words.txt` that we sampled from the Steemit dataset.

The `bench.go` shows how we used to benchmark desearch queriers.
Note that the dependency [Tachymeter](https://github.com/jamiealquiza/tachymeter) is required.

You can also use `ApacheBench`:
```shell
ab -n 10000 -c 1 http://127.0.0.1:12000/query/steemit/page/1
```

To increase the concurency level of the client, you can update the the thread pool number in `client/contrib/SimpleWeb/server_http.hpp`:
```shell
346    std::size_t thread_pool_size = 1;
```