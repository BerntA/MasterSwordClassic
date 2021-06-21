# Central Hub
This is the new FN, a simple RESTFul API which only allow certain whitelisted addresses to communicate with it.
No additional auth is needed, whitelisted addresses can be added and removed dynamically.

SQLite is used to store all character data, this makes things as simple as possible, no configuration is required.
Backup routines will by default run every four hours.
