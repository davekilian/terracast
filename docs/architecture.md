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
* The Terrascale Visualizer, a high-performance in-browser rendering engine
* Terrascale Projects, a real-time collaborative, version-controlled studio for analysis project
* The Terrascale API, which provides frontend and backend programability for the above
* Terrascale, an in-browser webapp which unifies the experiences above

## Cloud Provisioning

Terrascale is a multi-tenant service through which customers manage single-tenant compute and storage primitives stored in a third-party cloud such as AWS, Azure or GCP. The model is similar to Snowflake's virtual warehouse concept. This gives customers fine-grained control over the compute-vs-cost tradeoff, and isolates compute- and I/O-heavy analytical queries to individual customers.

## Workload Assumptions

Terrascale is a hybrid transactional/analytical database with support for high-volume short-lived transactional queries in tandem with low-volume long-lived analytical (cross-tabulation) queries. Analytical queries can be run as a single finite request, or incrementally over real-time streaming data. We expect users implementing operational and monitoring workloads will 

Types of database queries we expect include

* **Point reads and writes**: TODO OLTP for backend data management
* **Time- and space-windowed queries**: TODO OLTP for frontend visualization
* **Analysis**: TODO large-scale OLAP reads
* **Complex transactions**: TODO occasional large-scale updates and schema changes

## Storage Model

## Transactions and Concurrency Control

## Distributed SQL Queries

## Geospatial

## Incremental Queries and Real-Time Streaming

## Imperative Language Support

## Embeddable Visualizer

## Editor, Projects and Version Control

