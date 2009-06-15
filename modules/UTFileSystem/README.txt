

Node Inheritance Diagram
----------------------------------------------------------------


                      +- SymlinkNode
                      |
                      |       +- RootDirNode
                      |       |
                      +- DirNode
                      |       |
                      |       +- SpecialDirNode
                      |
              +- FileNode
              |       |
              |       +- SpecialFileNode
              |
      +- RefBlockNode
      |       |
      |       |       +- ZeroIndirectBlockNode
      |       |       |
      |       +- IndirectBlockNode
      |               |
      |               |       +- ZeroDoubleIndBlockNode
BlockNode             |       |
      |               +- DoubleIndBlockNode
      |               |
      |               |       +- ZeroTripleIndBlockNode
      |               |       |
      |               +- TripleIndBlockNode
      |               |
      |               |       +- ZeroQuadIndBlockNode
      |               |       |
      |               +- QuadIndBlockNode
      |
      +- DataBlockNode
              |
              +- ZeroDataBlockNode
