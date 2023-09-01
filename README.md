# Compile time reflection system

This little project shows how to implement a compile time reflection system for resource management.
The result can then be used for creating an execution graph for your multi-threaded system, 
like [entt::flow](https://github.com/skypjack/entt/wiki/Crash-Course:-graph#flow-builder) for example.


The output is basically a list of all accessed resources of one routine,
including accessed resources of all sub-routines.
So when I define a task for a multi-threaded system, I only list accessed
resources and called functions without the need to manually go into every
function to check on used resources.

You can find a complete example in the code.

### TODO
Filter out resources which are listed as read access but also exist as write access.
