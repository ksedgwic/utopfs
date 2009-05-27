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
    . ./env.sh
    rm -rf BLOCKS
    ../Linux.DBGOBJ/utopfs -M -d BLOCKS -P "" mnt

Mount existing file system in "BLOCKS":

    cd utopfs/Linux.WORKCFG
    . ./env.sh
    ../Linux.DBGOBJ/utopfs -d BLOCKS -P "" mnt
