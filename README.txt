Installing from RPM
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


Some Prerequisites
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


Running
----------------------------------------------------------------

Create file system in "BLOCKS" and mount it on "mnt":

    cd utopfs/Linux.WORKCFG
    rm -rf BLOCKS
    mkdir mnt
    . ./env.sh
    ../Linux.DBGOBJ/utopfs -f -M -F "myfs" -P "" BLOCKS mnt

Mount existing file system in "BLOCKS":

    cd utopfs/Linux.WORKCFG
    mkdir mnt
    . ./env.sh
    ../Linux.DBGOBJ/utopfs -f -F "myfs" -P "" BLOCKS mnt


Debugging
----------------------------------------------------------------

Run a unit test under the debugger:

    cd unit/Linux.WORKCFG
    . ./env.sh
    gdb python
    (gdb) r /usr/bin/py.test --nocapture test_fs_sparse_01.py

