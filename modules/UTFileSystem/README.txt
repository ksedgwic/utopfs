

Node Inheritance Diagram
----------------------------------------------------------------


                            +- SymlinkNode
                            |
                            |        +- RootDirNode
                            |        |
                            +- DirNode
                            |        |
                            |        +- SpecialDirNode
                            |
                   +- FileNode
                   |        |
                   |        +- SpecialFileNode
                   |
          +- RefBlockNode
          |        |
          |        |        +- ZeroIndirectBlockNode
          |        |        |
          |        +- IndirectBlockNode
          |                 |
          |                 |        +- ZeroDoubleIndBlockNode
   BlockNode                |        |
          |                 +- DoubleIndBlockNode
          |                 |
          |                 |        +- ZeroTripleIndBlockNode
          |                 |        |
          |                 +- TripleIndBlockNode
          |                 |
          |                 |        +- ZeroQuadIndBlockNode
          |                 |        |
          |                 +- QuadIndBlockNode
          |
          +- DataBlockNode
                   |
                   +- ZeroDataBlockNode


Caching Policy
----------------------------------------------------------------

All dirty nodes are held (by handle) in the tree.

Clean nodes are held in the MRUCache according to availability.

During tree traversal node handles are held to traversed subtrees.
