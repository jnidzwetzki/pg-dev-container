# Extension with a Coding Error

This directory contains a simple extension that defines the function `code_mistake`. Unfortunately, this function contains a bug.

### Current result

The `make installcheck` command is failing.

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

$ cat regression.diffs
[...]
@@ -21,12 +21,7 @@
 (1 row)
 
 SELECT code_mistake(100);
-INFO:  all "101" elements are contaied
- code_mistake 
---------------
- 
-(1 row)
-
+ERROR:  value "64" is not part of the bitmap set
 SELECT code_mistake(99999999999999999);
 ERROR:  function code_mistake(bigint) does not exist
 LINE 1: SELECT code_mistake(99999999999999999);
```

### Expected result
The `make installcheck` command should pass. 

_Note:_ The bug is in the program logic, the test specification is correct.

```
$ make installcheck
[...]
# +++ regress install-check in  +++
# using postmaster on Unix socket, default port
ok 1         - 00_mistake                                  3 ms
1..1
# All 1 tests passed.
```

### Your task
Your task is to understand what the `code_mistake` function of the extension is supposed to do, find the bug, and fix it. In addition, you should be able to explain why the problem happens with the value `64` and all lower values work properly.