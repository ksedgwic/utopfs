

Node Inheritance Diagram
----------------------------------------------------------------


                       +- SymlinkNode
                       |
                       |     +- RootDirNode
                       |     |
                       +- DirNode
                       |     |
                       |     +- SpecialDirNode
                       |
                +- FileNode
                |      |
                |      +- SpecialFileNode
                |
     +- RefBlockNode
     |          |
     |          |               +- ZeroIndirectBlockNode
     |          |               |
     |          +- IndirectBlockNode
     |                          |                +- ZeroDoubleIndBlockNode
BlockNode                       |                |
     |                          +- DoubleIndBlockNode
     |                                           |                + - ...
     |                                           |                |
     |                                           +- TripleIndBlockNode
     |                                                            |
     |                                                            |
     |                                                            +- ...
     +- DataBlockNode
                 |
                 +- ZeroDataBlockNode
