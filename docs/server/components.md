# Components

The server uses a collection of gameplay-focused ECS components to represent all relevant state for simulation.\
These components contain only data and no logic.\
All logic is implemented by systems executed inside the server’s Game Loop Thread.

This section documents every component used on the authoritative server.\
Each component has its own subpage describing:

* its purpose,
* its fields,
* how it fits into the ECS pipeline,
* which systems read or write it,
* how it interacts with networking and snapshot generation.

List of components:

*
