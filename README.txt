Installing and Running from RPM
----------------------------------------------------------------

Install the software

    sudo rpm -Uv fuse-utopfs-0.2-1.fc10.x86_64.rpm

Make a mount point:

    cd /tmp
    mkdir mnt

Create and mount a utopfs:

    utopfs -M -F "myfsid" -P "mypassphrase" BLOCKS mnt

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

    # Ruby
    sudo yum install -y ruby rubygems
    sudo gem install rspec

    # py.test
    sudo yum install -y python-py
    sudo yum install -y python-setuptools

    # Google Protobuf
    sudo yum install -y protobuf protobuf-devel \
                        protobuf-compiler protobuf-debuginfo


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
