Terrascale is a cloud-native data storage, analysis and visualization engine with first-class support for geospatial data and real-time streaming. It includes

* An HTAP database with ANSI SQL support and Python programmability
* First-class support for geospatial vector data and raster imaging
* First-class support for incremental queries over real-time data
* A programmable, embeddable visualizer for scalar, vector-image and raster-image queries
* A collaborative editor with real-time, CRDT-resolved edits and version history
* Source data snapshots and version control

Terrascale is designed to support a variety of user scenarios online operations and offline analysis of real-world data. Key scenarios include operation support, such as tracking and building custom real-time visualization, monitoring and alerting for autonomous vehicles and robots operating in the real world, as well as scientific sensor analysis. Terrascale is not a GIS suite, but forms a potential basis for one.

Terrascale consists of the following software components:

* TerrascaleDB, a cloud-native distributed storage and query engine
* The Terrascale Tile Delivery Network (TDN), a CDN for raster image tiles
* The Terrascale Visualizer, a high-performance in-browser rendering engine
* Terrascale Projects, a real-time collaborative, version-controlled studio for analysis project
* The Terrascale API, which provides frontend and backend programability for the above
* Terrascale, an in-browser webapp which unifies the experiences above

## Cloud Provisioning

Terrascale is a multi-tenant service through which customers manage single-tenant compute and storage primitives stored in a third-party cloud such as AWS, Azure or GCP. The model is similar to Snowflake's virtual warehouse concept. This gives customers fine-grained control over the compute-vs-cost tradeoff, and isolates compute- and I/O-heavy analytical queries to individual customers.

## Database Workloads

Terrascale is a hybrid transactional/analytical database with support for high-volume short-lived transactional queries in tandem with low-volume long-lived analytical (cross-tabulation) queries. Analytical queries can be run as a single finite request, or incrementally over real-time streaming data. Users can independently scale compute and storage resources as a means to control the cost-performance tradeoff.

Kinds of queries we expect and optimize for include...

**Point reads and writes**: Workloads where users build their datasets from operational data such as sensor data ingestion or logging human interactions with software, will be characterized by point writes, each inserting a single row, potentially at a high rate with high parallelism. These workloads may be write-only, if users maintain their operational data in a separate database system and stream it to Terrascale, e.g. using Postgres's change data capture mechanism; alternately, these workloads may mix point reads and writes, if users maintain their operational data directly in Terrascale. These are expected to always occur within a single table partition.

**Time- and space-windowed queries**: As data streams into a Terrascale instance, we expect to see queries over small space and time ranges as human users export their data for human visualization or programmatic observability (monitoring and alerts). For most users, most of the time, we expect a low volume of these kinds of queries. However, this scenario needs to perform adequately under high volume, e.g. when a user's visualization get the "hug of death" from aggregators like Reddit and Hacker News &mdash; important early growth drivers for Terrascale itself. These are the mainline senario for real-time streaming incremental queries. Although windows may cross table partitions, we expect each query to cross few enough table partitions that clients can manage views themselves.

**Bulk import**: When users important data in bulk, either by importing an existing dataset into Terrascale or doing nightly dumps of operational data into Terrascale, workloads will be characterized by a few write transactions consisting of many, many rows. We do not expect these to happen often, nor do we expect more than one or two in parallel, but ingestion operations need to be atomic, restartable, and work across many table partitions.

**Analysis**: Users are expected to use their datasets, whether built operationally or via obtained via a bulk import, for cross-tabulating analysis. Although users are expected to run few analytical queries at a time, each of these queries may operate over large amounts of data cross all partitions of multiple tables, necessitating distributed joins and shuffles between nodes a la map-reduce. We expect many users to run analytical queries on a fixed schedule to warehouse data insights, but some users may wish to run incremental analysis on real-time streaming data for scenarios like anomaly detection for alerting.

**Complex updates**: As part of cleaning and maintaining large and complex datasets, we expect users will occasionally need to issue one-off 'corrections' to datasets. These are characterized by large, complex transactions which may read many rows, do some sort of computation, and push many updates. Table schema changes are another example of this kind of workload. These large transactions are challenging to handle well, since they may cross many table partitions and need to have full serializable semantics; however, because they are low volume and only happen occassionally, we don't optimize for these. We simply ensure these produce the correct results without causing excessive downtime for real-time and operational workloads.

**Bulk export**: 

## Storage Model

TODO this is layered. The lowest level is abstractions over block and object storage devices identified by URIs. The next level is a replication, erasure coding, checksumming and encrypting protection layer. The top level is higher-level storage primitives like logs and key-value catalogs for schema information.

## Indexing Structures and Aggregate Analysis

TODO this is an LSM-tree on replicated block storage which after checkpointing gets re-sorted into a Druid-like columnar inverted index optimized for aggregate functions. Most aggregate functions work just fine on the columnar inverted level, for example min just gets the lowest matching column value that has nonzero bits, count distinct and sum just look at number of instances of each column value, and mean is just sum divided by count. Some functions like ranks and percentiles need larger scans and may require more work, and/or approximation algorithms if we can find them (think like the quick select median of medians).

## Transactions and Concurrency Control

TODO this is an LSN-ordered publish log for write-only (WO) transactions, snapshot isolation for read-only (RO) transactions at any scale (point, window, analytical), and a predicate locking scheme for read-write (RW) transactions for the complex update scenario. WO transactions are written to one of the active write-ahead logs with a single record that describes and commits the transactions, whereas RW transactions commit incrementally, with the final commit piggybacked on the last incremental write. Recovery replays committed transactions and ignores uncommited ones; since this is an LSM, there is only a log, so there is nothing to undo.

The problem that needs to be solved here is how to rectify an ahead-of-time LSN ordering scheme with long running RW transactions with unpredictable predicate locking. If the predicates can be found ahead of time, we can ensure serializability easily, but if RW transactions can incrementally lock more and more predicates, it's not clear how to let that happen without locking the whole WO publish queue. An RW transaction can

But that's fine, I think we document two schemes. If it's possible to predicate lock for an RW transaction ahead of time, then the RW transaction simply acquires all predicate locks, waits for all preceding RW or WO transaction, then executes on its own isolated snapshot view of the world. If for some reason that is not feasible for some queries, those must lock the entire WO queue, which sucks but at least we can do some background buffering so when RW finally publishes, all WOs that were waiting on it show up instantly as well.

## Distributed SQL Queries

TODO this is raft for distributed commit of transactions. A good question is who plans and optimizes the query. It could be a random node in the network, or it could be the raft leader (if that wouldn't be a bottleneck, at least the leader is always up to date, right?) or it could be a multi-tenant cloud orchestration service sitting in front of the cluster (but I'd rather do everything in the cluster if possible, since we want to keep open the possibility of shipping a small cluster of nodes in a box).

## Geospatial

TODO this is columnar range breakdown to find matching items within bounding boxes, then analytical collision detection and other spatial algorithms to finish joins and whatnot

## Incremental Queries and Real-Time Streaming

TODO this is mostly a division into cases, what kinds of things the language supports and 

## Imperative Language Support

TODO I want this to essentially be a way of executing queries, without having to build/parse SQL strings, and help cast them to the types that users want to run calculations over, like numpy arrays or that nd thing that I remember seeing geo people really excited about

## Tile Delivery Network

TODO is this third party CDN? Nodes serving data from inside your network? Just putting tiles into objects and serving them over cloud URLs? Any processing that has to be done, you can do inside your processing cluster, right?

The other question here is tile processing, which is missing entirely from this document. Obviously this can't be SQL. Should it be shaders? Who executes the shaders?

## Embeddable, Programmable Visualizer

TODO this is a 2D rendering engine for tile and vector data that runs in a native app using C/Vulkan or on the web with Emscripten/WebGPU. 

## Projects and Version Control

TODO define some kind of project object model, introduce CRDTs for common operations, describe a database scheme for tracking version history

## Terrascale.io

TODO first party hosting for all this stuff. Billing and stuff.
