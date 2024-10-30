# Extension with a Coding Error

This directory contains a simple extension that defines the function `code_mistake`. Unfortunately, this function contains a bug. Your task is to understand what the function of the extension should do, find the bug, and fix it.

### Current result
```
$ make
$ sudo make install
$ make installcheck
[...]
# +++ regress install-check in  +++
# using postmaster on Unix socket, default port
not ok 1     - 00_mistake                                  3 ms
1..1
# 1 of 1 tests failed.

$ cat results/00_mistake.out
SELECT code_mistake();
ERROR:  value "64" is not part of the bitmap set
```

### Expected result
```
$ make installcheck
[...]
# +++ regress install-check in  +++
# using postmaster on Unix socket, default port
ok 1         - 00_mistake                                  3 ms
1..1
# All 1 tests passed.
```