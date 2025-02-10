# BlitzenContainerLibrary

Slightly modified version of the utility written in BlitzenEngine0. It is more more flexible than the original so that it can be used in other projects.

Replaces some important STL constructs, like std::vector and std::unique_ptr. It is less flexible however and not meant to be shipped, it is written for academic purposes and not a true replacement of the STL. 

But it works fine for Blitzen and I would like to expand it by using it on other projects.

To make it work, the memory manager needs to be intialized before any kind of allocation is done and its destructor needs to be called after everything is freed, otherwise it will assert. The Blitcl::MemoryManagerState* Blitcl::MemoryManagerState::s_pState static pointer needs to be redeclared with the above syntax on the file that initializes the memory manager. 
