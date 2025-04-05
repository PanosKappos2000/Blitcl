# BlitzenContainerLibrary

Slightly modified version of the library written in BlitzenEngine0.

Implements containers similar to the STL but with less functionality and obviously not as well made.

Uses custom allocation functions to log allocation sizes, and check for memory leaks at the end of the application.

For it to work a BlitzenCore::MemoryManager needs to be created before any container.
