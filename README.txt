
#### This needs updating pretty badly.

Installing and Running from RPM
----------------------------------------------------------------

Install the software

    sudo rpm -Uv fuse-utopfs-0.2-1.fc11.x86_64.rpm

Make a mount point:

    cd /tmp
    mkdir mnt

Create and mount a utopfs:

    utopfs -M 1000000000 -F "myfsid" -P "mypassphrase" BLOCKS mnt

Unmount it:

    umount mnt

Mount an existing utopfs:

    utopfs -F "myfsid" -P "mypassphrase" BLOCKS mnt

Turn up logging:

    utopfs -l 9 -F "myfsid" -P "mypassphrase" BLOCKS mnt


Building RPM packages from tarball
----------------------------------------------------------------

Setup your RPM build environment:

<http://www.bonsai.com/wiki/howtos/rpm/setup/>

Kick off the build:

    rpmbuild -tb utopfs-0.2.tar.gz

Harvest RPM from ~/rpm/RPMS ...


Development Prerequisites
----------------------------------------------------------------

    # Python
    sudo yum install -y python-devel

    # py.test
    sudo yum install -y python-py python-setuptools

    # Google Protobuf
    sudo yum install -y protobuf protobuf-devel \
                        protobuf-compiler protobuf-debuginfo

    # Fuse
    sudo yum install -y fuse-devel

    # OpenSSL
    sudo yum install -y openssl-devel

    # bdb
    sudo yum install -y db4-cxx db4-devel




Building
----------------------------------------------------------------

    make


Running Unit Tests
----------------------------------------------------------------

    make test


Running in Development Tree
----------------------------------------------------------------

Create a new filesystem:

    cd utopfs/Linux.WORKCFG
    ./runmkfs

Mount existing filesystem:

    cd utopfs/Linux.WORKCFG
    ./runmount


Debugging
----------------------------------------------------------------

Run a unit test under the debugger:

    cd unit/Linux.WORKCFG
    . ./env.sh
    gdb python
    (gdb) r /usr/bin/py.test --nocapture test_fs_sparse_01.py


Choosing BlockStore Implementations
----------------------------------------------------------------

The development tree currently defaults to the FSBlockStore.

The unit tests can be switched to an alternative blockstore by
editting the appropriate section of the unit/CONFIG.py file.

An alternative blockstore can be specified to the utopfs daemon using
the "-B" flag.  Example using the BDBBlockStore:

    utopfs -B BDBBS -F "myfsid" -P "mypassphrase" BLOCKS.db mnt


Ultra Primitive Garbage Collection
----------------------------------------------------------------

This no longer works from the BDBBS ... oh well ...

    touch BLOCKS/MARK
    ./refresh BLOCKS "" ""
    find BLOCKS/ -type f \! -newer BLOCKS/MARK -exec rm {} \;


Building ACE on OSX
----------------------------------------------------------------

    mkdir -p /usr/local/dist
    cd /usr/local/dist
    wget http://download.dre.vanderbilt.edu/previous_versions/ACE+TAO+CIAO-5.7.0.tar.bz2

    cd /usr/local
    tar xvfj /usr/local/dist/ACE+TAO+CIAO-5.7.0.tar.bz2

    export ACE_ROOT=/usr/local/ACE_wrappers

    cat > $ACE_ROOT/ace/config.h <<EOF
    #include "ace/config-macosx-leopard.h"
    EOF

    cat > $ACE_ROOT/include/makeinclude/platform_macros.GNU <<EOF
    include \$(ACE_ROOT)/include/makeinclude/platform_macosx_leopard.GNU
    EOF

    export DYLD_LIBRARY_PATH=${ACE_ROOT}/lib:${DYLD_LIBRARY_PATH}

    cd $ACE_ROOT/ace
    make

