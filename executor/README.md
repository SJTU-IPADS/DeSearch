# Executor

## Features

- Following the MISO (multiple-input-single-output) rule for the executors dataflow.
- Following the offset-by-one rule for the Kanban dataflow.
- Exploiting replicas to resist executor faults and data loss.

## Limitations

- Assuming an always-on manager, which runs on a trusted standalone machine.
- Using Kanban variables to simulate the blockchain interactions.
- Lacking standard remote attestation and key management.
- Lacking an incentive model.

## Crawler

- A kind of executor that downloads a batch of items and parses them into documents.

For Steemit, it gets a block from the Steem full node at one time, parses the block and filters out transactions that are expected to contain some posts. Finally, it uploads posts and witnesses to Kanban.

### Basic Workflow

```
Loop:
 glance at the epoch
 download blocks from Steemit
 parse blocks into posts
 upload posts to Kanban
 upload witness to Kanban
```

### Notes

We no longer support OpenBazaar because OB has been depleted since 2021 Jan: https://twitter.com/openbazaar/status/1346104369566121986

## Indexer

- A kind of executor that transforms documents into a stream of binary index and digest.

### Basic Workflow

```
Loop:
 glance at the epoch
 download documents from Kanban
 tokenize documents into index and digest
 merge index and digest into one binary
 upload binary to Kanban
 upload witness to Kanban
```

## Reducer

- A kind of executor that merges the new index and the global index.

## Querier

- A kind of executor that serves search requests in a privacy-preserving way.

### Security Analysis

Pros:

- ORAM to (partially) hide access patterns inside secure memory.
- Constant number and length of results to hide network patterns.

Cons:

- For multi-keyword searches, multiple rounds of ORAM fetch leaks info.
- Metadata manipulation using C++ STL leaks info.

## Manager

- A kind of executor that summarizes a root digest for witness keys in the current epoch and then increments the epoch.


# Term Explanation

## MISO

MISO stands for multiple-input-single-output. Desearch organizes the search dataflow to be a hash tree, unlike the blockchain's hash chain.

MISO produces exactly one output, hence it is intuitive to store the MISO-style witness using [out] as the key and the [witness] as the value in a key-value store, named Kanban.

## Offset-by-one

Desearch has a heartbeat called an epoch. When an epoch closes, a new root hash will summarize all the contents in this epoch. An executor in the pipeline only uses data from the last epoch.

## Kanban

Kanban stores both data and metadata. For data in a certain epoch, the corresponding witness summarizes their correctness. For metadata such as witnesses, the corresponding root digest summarizes their correctness.
The root digest is generated by hashing a sorted list of the keys to the witnesses.


# FAQ

- Will Kanban be a source of centralization?

Desearch needs a tamper-proof yet efficient storage layer. Its current form relies on cloud storage as Kanban, but can turn to IPFS as well. As desearch provides search services for DApps whose data sources are authenticated, if the cloud storage cheats, all executors can switch to other service providers and make the original cloud lose benefits. The root digests help to detect this. An incentive model may punish dishonesty using liquidated damages.

- Will Manager be a source of centralization?

Desearch is a permissionless network and allows any executor to become a manager. Its internal deterministic execution will guarantee new managers will do the same job. Desearch leverages "first writer wins" where replicated manager work is discarded and hence not rewarded.

- Will stateful executors harm freshness?

Queriers and auditors are (somewhat) stateful executors that cache prior snapshots. The client should obtain the global timestamp, namely, the epoch from the ledger, and then be able to invalidate the cache.
