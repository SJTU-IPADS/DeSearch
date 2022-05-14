# Native crawler

This folder contains original implementation of crawlers for OpenBazaar and Steemit.

## OpenBazaar

OpenBazaar is a P2P e-commerce marketplace upon the IPFS storage.

You need to become an OpenBazaar peer first. Download it from [here](https://github.com/OpenBazaar).

```shell
openbazaard start -d ./data --allowip=127.0.0.1 --verbose

python3 openbazaar-crawler.py
```

It will help you fetch as many listings as possible from the P2P network.

The sad news is that OB has been depleted since 2021 Jan: https://twitter.com/openbazaar/status/1346104369566121986.

## Steemit

Steemit is a social media platform upon the public Steem blockchain.

One way to get Steemit posts is to become a full node, and another is to access Steemit always-online full node. We choose the latter.

```shell
go steem-crawler.go &

python3 steem-check-post.py
```