# This is a hack to avoid escaping the len() call in every test.
#
# We need to escape it because m4 resolves it as an m4 macro.
#
# We can't remember why we need m4 at the moment, we used to think
# it was grand ...

def lenhack(x):
    return `len'(x)
