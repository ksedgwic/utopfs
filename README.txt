Some Prerequisites
----------------------------------------------------------------

    # Ruby
    sudo yum install -y ruby rubygems
    sudo gem install rspec

    # py.test
    sudo yum install -y python-py
    sudo yum install -y python-setuptools


Building
----------------------------------------------------------------

    make


Running
----------------------------------------------------------------

This simple script runs utopfs in the foreground with fuse debugging
enabled:

    cd utopfs/Linux.WORKCFG
    ./runutopfs
