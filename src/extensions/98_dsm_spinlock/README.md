## Session 1
```sql
template1=# select * from grab_spinlock(10000);
 acquire_time |  total_time  
--------------+--------------
 00:00:00.001 | 00:00:10.001
(1 row)

Time: 10003.378 ms (00:10.003)
```

## Session 2
```sql
template1=# select * from grab_spinlock(10000);
 acquire_time |  total_time  
--------------+--------------
 00:00:05.334 | 00:00:15.338
(1 row)

Time: 15337.814 ms (00:15.338)
```
