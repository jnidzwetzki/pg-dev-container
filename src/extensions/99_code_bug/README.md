# Extension with a Bug

This directory contains an extension that defines the function `count_via_bms`. Unfortunately, this function contains a bug.

### Current result

The `make installcheck` command is failing since the PostgreSQL server is crashing while executing the regression checks.

```
$ make
$ sudo make install
$ make installcheck
[...]
# +++ regress install-check in  +++
# using postmaster on Unix socket, default port
not ok 1     - 00_bms_member                             603 ms
# (test process exited with exit code 2)
1..1
# 1 of 1 tests failed.

$ cat regression.diffs
[...]
@@ -27,18 +27,7 @@
 (1 row)
 
 SELECT count_via_bms(100);
- count_via_bms 
----------------
-           100
-(1 row)
-
--- Test debug statements
-SET client_min_messages = DEBUG2;
-SELECT count_via_bms(100);
-DEBUG:  count_via_bms performed "101" iterations
- count_via_bms 
----------------
-           100
-(1 row)
-
-RESET client_min_messages;
+server closed the connection unexpectedly
+       This probably means the server terminated abnormally
+       before or while processing the request.
+connection to server was lost
```

### Expected result
The `make installcheck` command should pass. 

_Note:_ The bug is in the program logic; the test specification is correct.

```
$ make installcheck
[...]
# +++ regress install-check in  +++
# using postmaster on Unix socket, default port
ok 1         - 00_bms_member                                3 ms
1..1
# All 1 tests passed.
```

### Your task
Your task is to understand what the `count_via_bms` function of the extension is supposed to do, find the bug, and fix it. In addition, you should be able to explain why the problem happens with larger values and how all smaller values work correctly.
