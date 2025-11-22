# Components

The client uses a collection of visual and prediction-focused ECS components to represent all data required for rendering, interpolation, and local prediction.\
These components store only data and contain no gameplay logic.\
All client-side logic is implemented by systems running in the main game loop, such as prediction, interpolation, animation, and rendering.

This section documents every component used by the client.\
Each component will have its own subpage describing:

* its purpose
* its fields
* how it fits into the client ECS pipeline
* which systems read or write it
* how it interacts with snapshots, prediction, or interpolation

Components will be added and documented progressively as they are introduced in the client architecture.
