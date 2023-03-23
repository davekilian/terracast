# Terrascale: Planet-Scale Analytics

Terrascale is a cloud-native data storage, analysis and visualization engine with first-class support for geospatial data and real-time streaming. It includes

* An HTAP database with ANSI SQL support and Python programmability
* First-class support for geospatial vector data and raster images
* First-class support for incremental queries over real-time data
* A programmable, embeddable visualizer for scalar, vector-image and raster-image queries
* A collaborative editor with real-time, CRDT-resolved edits and version history
* Source data snapshots and version control

Terrascale is designed to support a variety of user scenarios involving real-world data and measurements, including online operations and offline analysis, all on a single dataset. For example, a customer might use Terrascale to build realtime monitoring, visualization and alerting for autonomous vehicles, then run large-scale analysis to mine their operational data for trend insights. Others might use Terrascale to integrate satellite raster data with on-the-ground sensors data collected in real time via an IoT network, to feed online or offline analysis and/or monitoring. Terrascale is well suited for geographic analysis scenarios, but it is not a full GIS suite; however, its extensibility model allows it to act as a potential basis for one.

Terrascale consists of the following software components:

* TerrascaleFS, a cloud-native distributed file system for unstructured data storage
* TerrascaleDB, a distributed HTAP SQL engine with first-class geospatial support
* The Tile Delivery Network (TDN), a CDN and processing engine for tiled raster images
* Terrascale's Visualizer, a high-performance in-browser 2D rendering engine and database client
* Terrascale Projects, a real-time collaborative, version-controlled studio for scientific analysis
* The Terrascale API, which provides frontend and backend programability for the above
* Terrascale, an in-browser webapp which ties these into one unified experience

Although the initial build of Terrascale will be optimized for cloud computing scenarios, we plan to design up front to make it feasible to deploy Terrascale on the edge, as (for example) a ruggedized box housing a small cluster of compute/storage nodes running Terrascale. We abstract away cloud-native storage systems and compute orchestration APIs to make it simpler to build a local clustering system, and we have designed Terrascale to work with a very small number of compute nodes from the start. Fully lighting up this scenario would require additional currently-off-roadmap work, such as a synchronization engine for moving portions of datasets between the edge and the cloud, as well as implementations of our storage-layer abstractions suitable for use on bare-metal clusters.

In this document, we will build Terrascale from the bottom up, starting from our cloud-native data management and tile rendering architecture, then moving onto client-side experiences such as the visualization engine and extensibility surface, and finally onto the Projects experience hosted on Terrascale's website.

## Cloud Provisioning

Terrascale clusters are small (<100 node) groups of virtual machines running in a third-party cloud such as AWS, Azure or GCP. Terrascale clusters are similar in spirit to Snowflake's virtual warehouse API concept. This gives customers fine-grained control over the compute-vs-cost tradeoff, and pushes the problem of isolating clusters down to the cloud on which the cluster runs. If Terrascale were instead a single multi-tenant cluster, we would be responsible for isolating node performance and ensuring data security boundaries between tenants; with this architecture, the cloud provider provides both guarantees to the extent applicable.

### Orchestration

Terrascale clusters are managed using a single, global, multi-tenant orchestration service that uses a globally distributed third-party cloud database to track the set of user accounts, clusters associated with accounts, and cluster geometries. The orchestrator models the individual nodes of the cluster and tracks persistent disk volumes and IP addresses which are associated with the node. Each node in a Terrascale cluster is deployed with a configuration file which notifies it of the orchestrator's endpoint and authentication information; nodes register with the orchestration service on boot, and in doing so receives a map of the cluster which is used to bootstrap peer-to-peer consensus locally within the cluster. Nodes send heartbeats to the orchestration service, allowing the orchestrator to detect problems and redeploy as needed.

On bare metal cluster scenarios, the machines are preconfigured with a list of each others' IP addresses, which is used to bootstrap consensus between each other. The orchestration layer then runs in the context of whichever node is currently leading consensus.

### Managing Secrets

Keys, certificates and other kinds of secrets needed to access cloud object stores and databases within a Terrascale cluster are stored in the orchestration service's cluster management databases. When a node within a Terrascale cluster needs access to an object store bucket or cloud database, it authenticates itself with the orchestration service by returning the authentication blob it was deployed with, and requests a temporary access token to the relevant object store or cloud database. The orchestrator authenticates and authorizes, generates a token which temporarily grants the bearer access to the requested resource, and returns it to the cluster node. The node marks the expiration time and auto-renews before the deadline to avoid service disruptions.

A similar handshake is used to manage client-side encryption keys. Nodes request the current encryption key for the cluster's data, bearing the same authentication information as mentioned earlier, as well as a temporary public key for the session. The orchestration service encrypts the cluster encryption key with the node's private key and returns it. The node uses its private key to decrypt this key, and then uses the key as the key-encryption-key for local encryption operations.

## Storage Interfaces

TerrascaleDB includes a thin storage abstraction layer responsible for interfacing with external storage APIs and network interfaces. This eases the process of moving Terrascale between cloud providers and potentially makes it simpler to light up custom hardware solutions and on-prem scenarios. The goal of this layer is to provide abstract semantics; additional services like encryption, checksumming and compression are handled at higher layers.

The storage layer defines three kinds of storage:

* **Block devices** provide a high-cost, low-latency, full-featured file system with write-in-place semantics. Some block devices are **durable**, making them appropriate for staging customer data; others are **volatile**, making them appropriate for cache-spilling, staging external writes and storing intermediate results. Each block device is privately owned by a single node in the cluster.
* **Object stores** provide low-cost, high latency buckets of write-once unstructured data objects. Depending on the implementation, object stores may be strongly consistent or eventually consistent; as such, the object store abstraction only provides eventual consistency. Object stores are globally visible across all nodes in a cluster and are often used for sharing data.
* **Catalogs** are small, strongly consistent, globally visible key-value stores. These are well-tuned for bookkeeping needs, such as storing lists of tables which exist or blocks in the TerrascaleFS file system. Metadata for objects stored in object stores is often stored in catalog, as the strong consistency guarantees of a catalog allows clients to detect eventual-consistency artifacts in the objects themselves. 

TerrascaleFS is layered on top of this model and provides a flat namespace of GFS/HDFS-like append-only blocks with several value-add features, such as encryption, compression, checksumming, replication, snapshots, caching and cost-tiering. Most data in TerrascaleDB is stored in a mixture of storage-layer catalogs and TerrascaleFS blocks.

In the cloud, durable block devices are cloud block stores such as EBS or managed disks accessed through a local file system. Volatile block devices are locally-attached disk instances, also accessed through the local file system. Object stores wrap cloud object stores like S3 and block blobs. Catalogs wrap cloud-native table storage like DynamoDB or Azure's tables. 

In the future, TerrascaleDB may be extended to run on small clusters of hardware nodes (e.g. an on-the-go ruggedized 'edge' offering). In this setup, durable block devices and object stores might be implemented as a custom replicated protection protocol over physical disks, with n-way replication for durable block storage and Reed-Solomon erasure coding for objects. Volatile block devices might be implemented as non-protected disk storage (no replication or erasure coding). Catalogs might be implemented using a third-party / open-source database running on each node in the cluster, or as decrees passed by a Raft/Paxos cluster.

Individual block stores, objects in object stores, and catalog tables are identified using URIs. Terrascale defines a set of custom URI scheme to identify TerrascaleDB data artifacts stored in various external stores:

| URI Scheme | Storage Type | Location | Where Stored                           |
| ---------- | ------------ | -------- | -------------------------------------- |
| `trfs`     | Block/Object | Local    | Files in a local file system directory |
| `trkv`     | Catalog      | Local    | Text file in local file system file    |
| `trec2`    | Block        | Cloud    | Amazon EC2                             |
| `trmd`     | Block        | Cloud    | Azure Managed Disk                     |
| `trpgblob` | Block        | Cloud    | Azure Managed Disk                     |
| `trs3`     | Object       | Cloud    | Amazon S3                              |
| `trblob`   | Object       | Cloud    | Azure Blob Storage                     |
| `trdyn`    | Catalog      | Cloud    | Amazon DynamoDB                        |
| `trtab`    | Catalog      | Cloud    | Azure Table Storage                    |

## Distributed File System

TerrascaleFS is a distributed file system which runs locally inside a Terrascale cluster, either on bare metal hardware or in the cloud, without the need for any specialized hardware or a central coordinator. It provides append-only semantics similar to the Google File System (GFS), Hadoop File System (HDFS) and Azure Storage Stream layer, but uses a simple, flat namespace of inode numbers instead of a hierarchical directory structure. TerrascaleFS can seamlessly encrypt, checksum, compress, tier, cache, replicate and erasure-code data within in the file system. 

Currently, Terrascale uses TerrascaleFS as the underlying storage for TerrascaleDB and raster image tiles. In the future, TerrascaleFS could be exposed to customers directly for custom (Hadoop/Spark/Storm-style) analysis over their structured database data, raster tiles, and anything else they store in the cluster's file system as unstructured file data. Isolating the file system from higher-level activites also makes it possible to add selective cross-cluster sharing or TerrascaleFS files, enabling potential data publishing, sharing and marketplace scenarios.

### API Semantics

TerrascaleFS is a distributed file system. Each Terrascale instance has its own private TerrascaleFS file system instance running inside the cluster. All data for a TerrascaleFS file system is stored externally in cloud block storage, object storage, and database tables.

Each **file** in a TerrascaleFS instance is an unstructured byte array of varying lengths. The caller can choose length limits for its files; typically, a file size limits are the range of 512MB-2GB. Each file is identified by a 32-bit **inode** number, which is unique across all files stored within a cluster's TerrascaleFS instance. Clients directly create, open and delete files using inode numbers; there is no directory hierarchy or file names. (However, it is possible in principle to bolt a naming scheme on top of TerrascaleFS, if ever needed, by mapping directories and names to inode numbers.)

TerrascaleFS is meant only to serve data inside a Terrascale cluster. Data is secured against external access in two ways: first, all cloud storage endpoints for database and object data are configured to require authentiction, and second, all file contents are encrypted client-side by Terrascale itself. Within a cluster, however, any authenticated and authorized node can open, read, write or delete any file in the cluster's local TerrascaleFS file system instance. TerrascaleFS does not implement a file permissions system internally.

There are two types of files in a TerrascaleFS file system. An **active** file is mutable and has a single writer. Each active file belongs to a single node in the cluster, and can only be read or written by that node. Active files may be stored in durable or volatile block storage, depending on the caller's durability requirements. In contrast, **sealed** files are immutable, and can be read by any node in the cluster. TerrascaleFS automatically tiers files, storing active files in low-latency high-cost block storage, and offloading sealed files to high-latency low-cost object stores. TerrascaleFS can also cache and prefetch sealed file data locally in volatile block storage for low-latency access.

The following table summaries active vs sealed file semantics:

|               | Active                   | Sealed                   |
| ------------- | ------------------------ | ------------------------ |
| Writes        | Append-Only              | Not Supported            |
| Reads         | Full scan                | Paged                    |
| Stored in ... | Block storage            | Object storage           |
| Visibility    | Private to a single node | Public to entire cluster |

Internally, a TerrascaleFS file is composed of small, independent **blocks**, often about 1-64KB in size. Each block is independently encrypted, compressed and checksummed; TerrascaleFS manages block-level metadata transparently to the caller; callers view a file as the concatenation of the contents of its blocks. Blocks are the smallest unit of data that can be read independently, since the entire block must be read from external storage in order to decrypt and decompress the block and verify its checksum. A single append operation can append one or more blocks as an atomic group to the end of a file. Once a block has been written, it cannot be overwritten; the only way to modify a block is to delete the entire file which contains the block.

### Namespace Management

Unlike GFS and HDFS, TerrascaleFS does not rely on a global coordinator to manage the namespace. Instead, namespace-level information is stored in an external catalog, which all peers in a Terrascale cluster manage together using atomic transactions. For more details about catalogs, see the Storage Interfaces section above.

Two catalog tables are used to manage the state of the TerrascaleFS file namespace. The first is a table of files, where row contains the file's inode number, URI to the underlying block or object storage location for this file, the active/sealed state, and the node which currently owns this file (for active files). File create and delete options are committed by inserting and deleting rows from this table, respectively. As an active file is sealed and offloaded, its state and, eventually, its URI are updated in this table.

A second table of unused inode number ranges is used to efficiently allocate and recycle inode numbers in a peer-to-peer manner. Each row of this table is represents a contiguous range of available inodes. To allocate an inode, a node chooses a row at random, takes the lowest inode from that row, and does a batch transaction which deletes the original range and inserts a new row for the unallocated remainder of the range. For example, if the allocator picks the row (100, 200), it might allocate inode number 100, delete the (100, 200) row and insert a (101, 200) row in its place. When a file is deleted, its inode number is recycled back into this table, by merging it with the neighboring range(s) if possible, or adding it as a standalone row otherwise.

### Block Format

TerrascaleFS files expose an API interface where a single file appears to be a byte array, consisting of the concatenated contents of each block that was appended to that file. Internally, the file system layer also must manage block-level metadata, such as checksums, encryption metadata such as initialization vectors, and so on.

TODO on active files, each block header appears prior to the block data on disk. An in-memory index mapping logical block addresses to physical locations in the file is built incrementally as the file is scanned; the first time the file is read, the fs internally scans for append blocks until it finds the data requested, and caches the index in memory.

TODO when an active file is sealed, it is rewritten in a cleaning process, which is described in the Sealing and Publishing subheading later. During this process, the block-level metadata is TODO written to a second log? Appended at the end? Prepended at the beginning?

TODO what is the granularity of encryption? Does every file need to store a key blob that's encrypted with the cluster's key-encrypting key?

### Append Buffering

TODO this is where we should talk about buffering to the SSD page size and holding append operations until the data is flushed, unless maybe it sometimes makes sense to request a 'best-effort' append that returns as soon as it's buffered. The flush timeout is configurable and early flushes lead to wasted space which is cleaned when the file is sealed and published

### Sealing and Publishing

TODO this is the first time we talk about durable block storage for active/private and object storage for sealed/public with optional caching through a volatile block device.

### Cached Reads and Prefetching

TODO we use an LRU cache for extents and probably don't prefetch anything? Think about this some more. Maybe we don't do caching at this layer and it's fine. Or maybe unifying cache management does make sense. I'm not really sure.

### Replication and Erasure Coding

TODO for cloud-native storage devices, we default to a single replica. For local development scenarios, we keep a single replica in the local file system. For bare metal clustering scenarios, we will eventually need to support configurable replication factors and erasure coding. We may also need configurable replication factors for raster tiles anyways.

### Snapshots and Export

TODO just hint at future work, we may want to support these kinds of features in order to export files to a central registry for sharing and marketplace scenarios which are currently off-roadmap.

## Database Engine

TerrascaleDB is Terrascale's massively parallel data management engine for structured data. It implements a hybrid transactional/analytical database with support for high-volume short-lived transactional queries in tandem with low-volume resource-intensive analytical (cross-tabulation) queries over scalar, 1D range, and multidimensional spatial/temporal data, with native support for spatial queries and spatial joins. Using a SQL interface, users can run queries on a finite point in time, or update continually over real-time streaming data. Users can independently scale compute and storage resources as a means to control the cost-performance tradeoff of their queries.

### Workloads

The following classes of database transactions are expected and thus optimized for:

**Point reads and writes**: Workloads where users build their datasets from operational data such as sensor data ingestion or logging human interactions with software, will be characterized by point writes, potentially at a high rate with high parallelism, with each write inserting a single row. From Terrascale's point of view, these workloads may be write-only (when users host their operations in a separate system and stream updates to Terrascale, e.g. using a mechanism like Postgres's change data capture), or they may be a read-write-mix if users directly host their operations in Terrascale. In general, the point reads and writes generated by this aspect of a customer workload will each be confined to a single table partition.

**Time- and space-windowed queries**: As data streams into a Terrascale instance, we expect to see queries over small space and time ranges as users export their data for human visualization or programmatic observability (monitoring and alerts). We usually expect these small windowing queries to run periodically at a low rate, but anticipate that sometimes these views may be "slashdotted" and thus see a high rate of read-only traffic. These queries may run finitely or use incremental streaming, and each query will usually cross only a small number of table partitions.

**Bulk import**: Customers using Terrascale for offline analytics may bulk-import data from other sources, either as a one-time event to populate a cluster or as part of a nightly ETL database dump. This kind of workload is characterized by just one or two write transactions each inserting many, many rows, crossing all partitions of a cluster. It is unusual for customers to run other kinds of queries while this is ongoing; for example, an ETL customer will likely load all data into Terrascale before starting any new analytical queries. As such, it is okay for a bulk import to cause a storm of traffic that doesn't settle until the import has completed. Bulk import operations need to be restartable, whether through a checkpoint/restart protocol or idempotency.

**Analysis**: Users are expected to use their datasets, whether built operationally or via obtained via a bulk import, for large cross-tabulating analysis queries featuring complex filters, grouping, aggregations, cross-table joins, and spatial joins. Although users are expected to run few of theses kinds of queries at a time, each may necessitate a large amount of compute, storage and network traffic. Additionally, analytical queries may either run in a finite point-in-time or as incremental realtime streaming; the latter is useful for scenarios like anomaly detection and alerting.

**Complex updates**: As part of cleaning and maintaining large and complex datasets, we expect users will occasionally need to issue one-off 'corrections' to their datasets. These are characterized by large, complex transactions which may read many rows, do some sort of computation, and push many updates. Data cleaning transactions and table schema changes are examples of this kind of update. These large transactions are challenging to handle well, since they may cross many table partitions and need to have full serializable semantics; however, because they are low volume and only happen occassionally, we don't optimize for these. We simply ensure these produce the correct results without causing excessive downtime for real-time and operational workloads.

**Bulk export**: Customers may wish to do large, point-in-time bulk reads to export data from Terrascale as part of a wider ETL process that copies or backs up Terrascale data to a separate system. Like bulk import, exports need to provide consistency and restartability, whether the latter is achieved through idempotency or a checkpoint/restart protocol.

Data types include scalar values, one-dimensional ranges, n-dimensional spaces and time. Although there is nothing stopping users from placing JSON or XML data in string columns, TerrascaleDB does not currently include first-class support for semi-structured data.

### Architecture

TODO a draft scheme from yesterday: hash-partitioned range trees

- L0 is a hash-partitioned range index
- The database space is partitioned at L0 by hash on the primary key
- The L0 node set can scale elastically using consistent hashing
- At L0 the trees themselves are still sorted and support range queries
- A distributed versioning system for cross-partition snapshot isolated reads
- Individual L0 trees are multiversioned according to distribution
- A Paxos atomic commit scheme for cross-partition updates
- L1 trees are served by the L0 nodes using the same partition scheme
- L0 nodes transparently share or combine L1s after repartitioning
- L2 trees are served by the analysis nodes with size-based range partitioning
- L2 trees include row indexed and inverted column indexed representations
- A load balancer assigns queries to nodes arbitrarily for coordination
- Transaction coordinators contact L0-1-2 partitions as needed
- Realtime routing system integrated at the L0 layer
- Distributed transaction to replicate routes across all hash partitions
- Like druid, our node role names are realtime (L0-1) and historical (L2)
- Point read by primary key contacts realtime by hash, historical by partition map
- Other reads go to all realtimes, associated historicals, snapshot isolated
- Single insert/deletes directly contact the associated realtime node
- Bulk insert/deletes segment by realtime, executed as a distributed transaction
- Primary-key updates contact realtime node, which reads L2 inline for RMW
- All other updates use same path, but for all realtimes as a distributed transaction

## Row Stores



---

TODO the original TRFS grew up as ideas needed to manage this row store; now those need to be teased apart. This section will be mostly the same as before, but a lot of the details have been moved to the FS layer

---



TerrascaleDB takes a nod from WiscKey by storing database rows and index trees separately. That is, rows do not appear inside b-tree or LSM tree pages; instead, rows are stored in a separate row store, and index trees reference rows via an external identifier. After a query identifies result rows using a primary or secondary index, it issues additional reads to fetch the rows themselves from the row store. The row store is not modified when an index is checkpointed or merged; instead, row deletions and overwrites are handled with an independent copy-forward garbage collection scheme.

As noted in the literature for WiscKey, this scheme lowers write amplification, since rows do not need to be rewritten on checkpoint or merge, but does so at the expensive on increased read amplification, since rows are no longer found inside the LSM tree itself. In fact, a large number of reads may be issued after an index lookup if index-adjacent rows are scattered in the row store; in effect, the only 'primary index' on rows becomes the internal lookup table within the row store itself, and all database-level indexes are effectively 'secondary indexes' which map a primary key or other attribute to row identifiers. This tradeoff is considered a good fit for flash memory, which performs poorly and wears quickly on write-heavy workloads, but handles highly parallel random reads well. Separating rows from index is also useful for managing write- and space-amplification in Terrascale's dual-indexing strategy, which will be discussed later.

### Basic Structure

The row store is an unordered collection of append-only **segments**, which contain full rows that have been written to the database. Segments are row-oriented storage; each row appears as a tuple of values stored adjacently somewhere within the segment.  Each segment is an independent unit of storage with its own local address space. Any number of segments can be written to simultaneously. Segments are append-only: you cannot delete or overwrite existing data within a segment, but you can delete the entire segment at any time.

A segment can be **active** or **sealed**. While all data stored in a segment is immutable, only active segments admit appends of additional data. Sealed segments are completely immutable. Active segments are local to the node which created them, and are stored in node-specific durable block storage. When the segment reaches a size limit or otherwise needs to be shared with other nodes, it is sealed and then offloaded to object storage. From there, any node can page in the immutable segment from object storage, possibly caching it in memory and/or local volatile block storage. 

The target size limit for a segment &mdash; that is, the size at which a segment is sealed and offloaded to object storage &mdash; should be somewhere in the range of 256MB-2GB. Segments need to be large enough that the system does not need to manage an excessively large number of segments, but should also be small enough to admit short garbage collection passes with low churn.

Active segments are initially staged in durable block storage (see Storage Model above). When a segment is sealed, it is lazily offloaded to object storage, and may be read back in through a volatile block storage cache. The database catalog is used to track all segments, active or sealed, which currently exist, as well as their current locations in durable block or object storage.

### Identifiers and Sequencing

Each row stored in a row store has a 64-bit **row identifier** or **rowid** which is globally unique for a single TerrascaleDB database. The mapping from rowid to row value tuple is immutable once established; if a row is updated, it is rewritten as a new row with a new rowid, and the old rowid is later garbage-collected. Similarly, when the garbage collection process copies a row forward, it allocates a new copy of the same row with a new rowid, and repoints any index references to the old rowid to the new rowid.

A 64-bit rowid is partitioned into two 32-bit sub-identifiers. The 32 high bits of a rowid are the **segment identifier** or **segid**, which identify a single segment in the row store. All rows in a single segment implicitly share the same segid, so this portion of the rowid is not persisted per-row within a segment. The 32 low bits of a rowid are the **segment-local row identifier** or **localid**. These localids are allocated in monotonically increasing order as rows are appended to the segment, and are implicit by the relative ordering of rows within the segment: the first row in the segment has localid 0, the second has localid 1, and so on.

A database-global catalog table is used to track all segments, active or sealed, which exist for the database. This table is indexed by segid, and contains information like the current location of the segment in block or object storage, which node owns the segment (if still in block storage), its active-or-sealed state, and any bookkeeping information which is needed during the process of offloading a recently-sealed segment from block to object storage, and any statistics maintained for garbage collection, such as occupancy.

A second database-global catalog table is used to track contiguous ranges of available segids. Initially, this catalog has a single free range from 0 to `UINT_MAX`; segids are then allocated by reducing the low end of this range, and when segments are garbage-collected, their segids are recycled for reuse by returning them to this pool. When a segid is returned to this table, it is merged with one or two adjacent ranges, if possible, or otherwise, the segment is written as a new range in the table.

### Access Patterns

Active and sealed segments support different access patterns tailored for different use cases:

Active segments are intended for write-ahead and recovery scenarios. These segments are writable, but private to a single node, and are stored in block storage devices owned by that node. Writers can insert additional global and row-local "opaque" data that is not parsed by the row store layer itself, which is useful for storing bookkeeping information about transactions for write-ahead log recovery. An index which maps (row localid) to (byte offset of row within segment) is maintained in memory for active segments, but not on disk (doing so would cause fragmentation and frequent read-modify-write cycles on flash storage). Because there is no persistent index for an active segment, the only way to load an active segment is to read the entire thing from start to finish. This is usually required during write-ahead log recovery anyways.

Sealed segments are intended for long term storage and large-scale parallel queries. These segments are immutable, publicly visible to all nodes in a TerrascaleDB cluster, and are stored in global object storage. A persistent index from (row id) to (byte offset of row within segment) is persisted to the end of the segment when it is offloaded to object storage, allowing ranges of sealed segments to be demand-paged without reading the full segment end-to-end. All opaque data that was inserted into the segment when it was active is cleaned during the process of offloading the segment to object storage. The garbage collector only considers sealed segments when choosing candidate segments to garbage-collect.

The following table summarizes the capabilities and attributes of the two different kinds of row store segments:

|                    | Active                     | Sealed                               |
| ------------------ | -------------------------- | ------------------------------------ |
| Writes             | Append-Only                | Not Supported                        |
| Reads              | Full scan                  | Paged                                |
| Stored in ...      | Block storage              | Object storage                       |
| Cached             | In memory (resident)       | Memory and volatile block (spilling) |
| Visibility         | Private, owned by one node | Public, readable by all nodes        |
| Indexed            | In memory                  | On disk                              |
| Garbage collection | Never a candidate          | Always a candidate                   |

### Indexing

As previously mentioned, each row store segment, active or sealed, has an associated B+ tree index which maps each row's localid to its corresponding byte offset within the store. The indexing layer's memory table, row table and columnar tables all refer to rows by their global rowids. The B+ tree index inside the row store completes the mapping from rowid to physical location where the full row is stored.

For active segments, the segment index is maintained in memory. This is considered acceptable because the size of an active segment is limited, in practice, by the amount of time we're willing for a table partition to 'go offline' while the recovery process is reading rows back from disk; the segment index is an order of magnitude smaller, such that it easily fits into memory. A 1GB segment consisting of 128-byte rows has about 8 million rows; if each is indexed as a single 32-bit offset, the size of the index need only be roughly 32MB. Maintaining the index only in memory works because active segments are always reloaded in their entirety as part of recovery anyways.

For sealed segments, which may be demand-paged and should not be memory-resident, the index is stored directly in the segment file in object storage, immediately following the region of the segment which contains rows. Each sealed segment object file ends with a fix-sized footer which points backwards to the row array and the root page of the segment index; when a segment is requested, the cache manager reads the footer (if not already cached) to find the root index page, reads the root index page (if not already cached) to find relevant director and leaf pages, and so on.

Each leaf page consists of a page header, which includes (among other things) the localid of the first row referenced by this index page, and the number of rows indexed by the reference page. The rest the page consists of an array of 32-bit file offsets, where the first 32-bit value is the file offset of the row with localid (pagehdr->localid), the second is for the row with localid (pagehdr->localid + 1) and so on. The use of compact, fixed-size offsets allows the page to be binary-searched for a given localid.

In memory, internal pages store arrays of (localid) to (cache handle identifier) for child pages; on disk, internal pages are arrays of (localid) to (32-bit file offset of child page) instead.

### Write-Ahead

TerrascaleDB uses row stores for table-level write-ahead logging. When a row is inserted or updated, the full row value is appended to an active segment in the row store; when a row is deleted, a tombstone record is appended as opaque data instead. Each record includes opaque data which specifies transaction metadata and ordering information, which is used during recovery to rebuild all in-memory index state, undoing and redoing transactions as needed. Checkpoint state for batch inserts or updates, as well as more complex multi-row transaction checkpoint states, are also appended as opaque data.

The row store is responsible for buffering pages in memory and holding transactions until those pages are flushed durably to durable block storage. If an append to an active segment arrives while there is no active buffer, a new buffer is allocated, and a timer is started; when the buffer is filled or the timer elapses, the full page is flushed to durable storage, whether or not it is full. All transactions which were waiting for the flush are then released. Always flushing full pages at a time improves flash write latency (at the tail) and wear, as doing so eliminates the need for the FTL to perform internal read-modify-write cycles at the page level. Unfortunately, since these devices do not publish page sizes to the operating system, the size of a flush buffer must be configured manually.

Opaque data written to an active segment, as well as unused buffer space resulting from buffers that were flushed before they were completely full due to the flush timer, are all cleaned from the segment during the seal-and-off-load process, which is described next.

### Sealing and Offloading Segments

Active row store segments are usually sealed as part of checkpointing. During this process, the row store segments which contained the active write ahead log are sealed by updating the state of the segment in the catalog; in the same transaction, a new checkpoint LSM tree is registered with the catalog, and a new active row store segment is allocated to serve future write-ahead requests. After this, the sealed block-store segment is rewritten in object format in volatile block storage, and then the resulting buffer is uploaded to a new object store object. Once this is complete, the catalog is again updated to repoint the segment to its object store version, allowing the durable block storage copy to be deleted and the space to be reclaimed.

To offload a segment, the segment file is reread from durable block storage start-to-finish, as if recovering a write ahead log. Each row identified is written out to a new segment file in volatile block storage. Only rows are copied forward; opaque data as well as unused space from timed write-ahead buffer flushes are skipped. As rows are appended to the new file, their byte offsets are appended to B+ index pages in memory. Once all pages have been written out, the B+ pages are appended to the end of the file, and a fixed-size file-level footer with sequence numbering metadata and pointers to the row section and root index page is appended to the end of the file. The final file is then uploaded into object storage. Finally, to close the loop, the catalog is updated to repoint the logical segment to its public object-store representation rather than its private durable block-store representation. The latter file can then be deleted to reclaim space.

## Indexing Structures







---

TODO at some point while I was drafting this, I decided to go with a full dual-indexing route instead of the original plan, which was to use traditional LSM up until we have a checkpoint or two of data worth merging, at which point the amount of data we're handling becomes large enough that it's worth dual-indexing into row and columnar storage. We need to get back to that. In particular, it's an important part of the distribution and partitoining strategy, since you basically have (small sets of recent data where ingestion performance is key and scanning reads are okay since data is small) coupled with (large sets of historical data where we have time to ingest and want a variety of read queries to be well-supported). This split has implications both for indexing and distribution. Using LSM levels with our single-writer-active/global-reader-sealed segments makes this fairly natural, you emit data out of the write-focused database into globally visible object storage on checkpoint, and then the read-focused indexers asynchronously ingest and merge that data, completing the whole LSM handshake. It's all one big LSM process, but with the checkpoints partitioned one way, and the anchors partitioned another way.

I think what I want here is

* We log rows directly to the row store as described at length above
* A row index with LSM levels L0, L1 and L2, horizontally partitioned using b-tree-like range splits and merges (like cockroachdb)
* A column index that is built only by consume L1s to build L2 inverted indexes asynchronously with a similar split/merge strategy
* Possibly a separate lazy-built 'index-only' journal that just stores primary key to rowid mappings for faster recovery
* Probably multiversioning support on the L0 row index to support long-running transactions (not needed for fast data scans though)
* Spatial bounding box queries are scans over L0-1 data and column unions over L2

Do we want to partition the entire row index so a single node owns the whole thing for a key space? That's very reasonable in terms of wanting to answer point queries by talking to a single node. But another design that may be feasible is a hash-indexed frontend that ingests small amounts of live data, over which we accept most query types other than point lookups are full scans; then have L2 row index partitions and L2 columnar index partitions all consume incoming hash checkpoints. One thing that makes this attractive is we can put the stream-routing logic in the hassh partitioned live data frontend layer thingamajig and rely on that to scale as horizontally as you want. In fact, you could go completely overboard and just round robin to the 'live' layer as if it's a load balancer ... but then every read has to consult every replica which probably isn't a great. 

I guess really the point of that last question is that the exactly indexing scheme has to mesh with the exact partitioning scheme.

In that light, one thing worth noting is that we don't want to expose all our internal merge progress state to our clients, which means we're compiling queries on our end, which means we load balance queries essentially randomly across the cluster, which means even transactional queries are being planned on a different node than the one(s) storing the affected partition. If we already have to pay a network hop to get from the query managing node to the node(s) which own the partition(s) affected by the transaction, then having more different nodes to consult is more a question of tail latency than anything else. So that's a good argument for letting the L0-1 live data get partitioned independently of L2 data, which opens the door to maybe a consistent hashing approach.

Oh, and if we keep the idea of range-partitioning the L0-1 data, there's no reason the node which owns L0 data can't also cache the relevant L2 trees locally ... they're immutable, and this node is the one that kicks off L1->2 merges by publishing its L1 to the historical nodes, so it knows which L1-2 trees are needed to complete its current view of the world. "Of course I know him. He's me."

---















---

TerrascaleDB indexes data using a log-structured merge strategy inspired by ideas from Apache Druid and WiscKey.

Like in WiscKey, new rows are logged to a dedicated 'row store' in arrival order, are indexed externally using log-structured merge trees, and are cleaned up using copy-forward garbage collection. Storing rows out-of-page with respect to the log-structured merge tree index incurs additional read amplification (since rows are not in the tree and must be fetched with an additional read after the tree lookup) but allows for lower write amplification (since the rows themselves do not need to be rewritten with the rest of the tree on checkpoint and merge). This design is a particularly good match for modern flash memory, which generally provides very good latency/throughput on high-queue depth random reads, but is prone to high write latency tails and early wearout for write-heavy workloads.

TerrascaleDB uses a dual indexing strategy over this arrival-order row store. One of the indexes is a traditional row-oriented log-structured merge tree which maps primary keys to the corresponding location in the row store. The other index is a columnar inverted index inspired by Apache Druid. The row index is well-suited for answering OLTP queries that request individual rows or small ranges, and the columnar index is well-suited for OLAP (cross-tabulation) queries that feature large-scale aggregation and grouping with potentially complex multidimensional filters and large joins over many tables.

Dual-indexing enables hybrid transactional/analytical processing, at the cost of roughly doubling the write- and storage-amplification overhead compared to a row-only or column-only index. Storing (relatively large) row payloads out-of-page in a separate, garbage-collected row store as suggested by WiscKey reduces the write- and space-amplification overhead of dual-indexing to what we believe is a manageable level.

TerrascaleDB's indexing layer supports range types, and automatically breaks down overlapping ranges in the index transparently, on-the-fly. The columnar index supports efficient lookup of objects which intersect an n-dimensional bounding box specified by one range along each axis. N-dimensional bounding box queries are used by the query layer to generate candidate result sets, which are further refined using fine-grained collision detection algorithms for spatial queries and spatial joins.

This section begins with an overview of the different on-disk structures that store and index data in a single TerrascaleDB table. We then discuss algorithms for building and garbage-collecting these structures, and for offloading into object storage. Finally, we discuss mainline query paths for scalar data and spatiotemporal bounding boxes for transactional and analytical processing workloads.

### Memory Tables

TODO I think what I want to do here is to keep row store pages referenced by the memory table resident in memory, and then have the memory table be the same index in an in-memory multi-version binary search tree that it is on disk. Multiversioning with columnar indexing may be quite hard because there's no longer a single bitmap; there's one per version, or there's one bitmap with versioned bits, neither of which is space-efficient. Maybe we can get away with a row-only memory table and emulate columnar by full memory table scans? Maybe we build a columnar memory table lazily? Maybe we have a traditional column store memory table, but an inverted index style on disk?

### Row Tables

TODO this is probably the easiest part of the system, each of these is just a dense (pages fully utilized) B+ tree on disk, where the keys in the tree are row primary keys and the values are row IDs of rows in the row store.

TODO it's probably worth pointing out at some point that it's possible for users to ask us to maintain a secondary index, and we do that as easily as creating a primary index. There's an opportunity to elide an entire row index if it's a single column we already indexed, but that's a micro-optimization we can look into later.

### Column Tables

TODO this is an LSM tree where keys are unique column values that appear in the tree, and the values are row ID lists &mdash; can be in-page for just a few results or an external pointer to a dedicated roaring bitmap page if there's more than just a few. Each entry also stores an up-to-date entry count. For differential trees, you also need a way to specify a set of rows that were deleted from the system so they can't be found in older trees. This should be a separate, optional delete list which is again either a sparse list or a full roaring bitmap.

So far it's seeming likely we'll be using 64-bit row IDs, in which case https://github.com/RoaringBitmap/RoaringFormatSpec specifies extensions for 64-bit row numbers.

### Write-Ahead, Checkpoint and Merge

TODO write-ahead commits to a row store page in memory; the transaction then gets blocked on the page's flush event. That flush comes either when the page is full, or after the first transaction has been waiting 'too long.' Timed flushes can lead to a single page being flushed multiple times, e.g. every time a timeout occurs or on the final flush once the page is full. We handle this by rewriting in place. We put the raw row value as the row entry in the store, and we put our own recovery information in the opaque entry.

All on-disk row- and column-trees are LSM, and work via standard inorder checkpointing/merging algorithms. Mention that we assume we're checkpointing to local volatile SSD storage before we offload to object storage. As such, a reasonable merge policy is basically just "min completion rate with deadline."

### Garbage Collection

TODO this is ... hard, as always. The simplest way to do this is naive mark and sweep: walk the live row index to build occupancy statistics (how many bytes of live data divided by how many bytes), pick which object-store objects we want to collect, do another index walk to identify rows that need to be rewritten with a new row ID

### Queries on Scalar Data

TODO this is pretty standard LSM for transactional. For analytical, we'll discuss it below in the aggregate analysis section

### Range Breakdown and N-D Bounding Box Queries

TODO this is obvious for the column store, each unique range after breakdown lists a bitset of objects that fall along this section of the axis, and a bounding-box query is just an intersection on each axis. The only interesting question here is, can a range appear in a primary key? If so, how does the row index handle that?

## Aggregate Analysis

TODO Anything you can do on unsorted columnar data, you can also do on an inverted index. When you come across a column value in an inverted index, you repeat whatever you would have done with your columnar forward index as many times as there are matching bits in the bitset.

More concrete examples: min just gets the lowest matching column value that has nonzero bits, count distinct and sum just look at number of instances of each column value, and mean is just sum divided by count. Some functions like ranks and percentiles need larger scans and may require more work, and/or approximation algorithms if we can find them (think like the quick select median of medians).

## Distribution

TODO I don't have the full story here, bu what I think makes sense is a dual strategy that takes advantage of immutable LSM tree checkpoints of row data passed through object stores as the "shuffle" step for building a long-term store partitioned different than a short-term store. Basically, short term data is mostly in memory, partitioned to facilitate fast ingest, and doesn't have very good read locality (may involve large scans across all shards for read range queries); but then long-term data is compressed in object storage and read in through disk caches, and sorted into large, read-friendly ranges. Long-term data is all LSM trees (of primary keys to rows, or of column values to bitmaps) so probably what makes sense here is range-based partitioning that do neighbor splits and merges kind of like pages of a classical b-tree, as inspired by CockroachDB and probably used earlier by many other systems. I'm not yet certain how the original in-memory representation should be partitioned. I'm tempted to hash primary keys and rely on ask-everyone style queries for all range queries and all non-primary-key queries, but I can't convince myself that's not crazy bad for live streaming. Unless, ofc, live streaming is a separate pipeline-based routing service that doesn't use any of this stuff. Decisions, decisions ...

Different columns are stored on differnet nodes, so the way we do filtering most likely is to locally union all relevant per-value bitmaps, send that back to the query planner and let the query planner union/intersect the relevant bitmaps as needed, then have a next phase where it pushes bitmaps back down to the indexing layer to have individual partition nodes run aggregates over the given filter. Wait, we're not really set up for that. What am I talking about. I think we have to predicate pushdown to the column layer. No that's not right either. I seem to have confused myself lol.

## Transactions and Concurrency Control

TODO this is an LSN-ordered publish log for write-only (WO) transactions, snapshot isolation for read-only (RO) transactions at any scale (point, window, analytical), and a predicate locking scheme for read-write (RW) transactions for the complex update scenario. WO transactions are written to one of the active write-ahead logs with a single record that describes and commits the transactions, whereas RW transactions commit incrementally, with the final commit piggybacked on the last incremental write. Recovery replays committed transactions and ignores uncommited ones; since this is an LSM, there is only a log, so there is nothing to undo.

The problem that needs to be solved here is how to rectify an ahead-of-time LSN ordering scheme with long running RW transactions with unpredictable predicate locking. If the predicates can be found ahead of time, we can ensure serializability easily, but if RW transactions can incrementally lock more and more predicates, it's not clear how to let that happen without locking the whole WO publish queue. An RW transaction can

But that's fine, I think we document two schemes. If it's possible to predicate lock for an RW transaction ahead of time, then the RW transaction simply acquires all predicate locks, waits for all preceding RW or WO transaction, then executes on its own isolated snapshot view of the world. If for some reason that is not feasible for some queries, those must lock the entire WO queue, which sucks but at least we can do some background buffering so when RW finally publishes, all WOs that were waiting on it show up instantly as well.

Also, I decided to put off discussing the recovery scheme until here.

Also, this is the point where we have to talk about local vs global transactions

## Distributed Queries

TODO queries are load-balanced randomly across nodes; whichever node receives the query becomes the coordinator. This is also where we need to go through the different classes of expected query types and walk through how the structures above are used.

## Geospatial

TODO this is columnar range breakdown to find matching items within bounding boxes, then analytical collision detection and other spatial algorithms to finish joins and whatnot

## Incremental Queries and Real-Time Streaming

TODO this is mostly a division into cases, what kinds of things the language supports and how we update these over time.

## Imperative Language Support

TODO I want this to essentially be a way of executing queries, without having to build/parse SQL strings, and help cast them to the types that users want to run calculations over, like numpy arrays or that nd thing that I remember seeing geo people really excited about

## Tile Delivery Network

TODO is this third party CDN? Nodes serving data from inside your network? Just putting tiles into objects and serving them over cloud URLs? Any processing that has to be done, you can do inside your processing cluster, right?

## Tile Processing Engine

TODO obviously this can't be SQL. Should it be shaders? Who executes the shaders?

## Visualization Engine

TODO this is a 2D rendering engine for tile and vector data that runs in a native app using C/Vulkan or on the web with Emscripten/WebGPU. 

## Projects and Version Control

TODO define some kind of project object model, introduce CRDTs for common operations, describe a database scheme for tracking version history

## Terrascale.io

TODO first party hosting for all this stuff. Billing and stuff.
