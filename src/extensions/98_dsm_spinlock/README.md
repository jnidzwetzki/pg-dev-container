## Preparation
```sql
db1=# CREATE EXTENSION dsm_spinlock;
CREATE EXTENSION
```

## Session 1
```sql
db1=# select * from grab_spinlock(10000);
 acquire_time |  total_time  
--------------+--------------
 00:00:00.001 | 00:00:10.001
(1 row)

Time: 10003.378 ms (00:10.003)
```

## Session 2
```sql
db1=# select * from grab_spinlock(10000);
 acquire_time |  total_time  
--------------+--------------
 00:00:05.334 | 00:00:15.338
(1 row)

Time: 15337.814 ms (00:15.338)
```

## pg_stat_activity
```sql
db1=# select pid, query_start, query, wait_event from pg_stat_activity where datname = 'template1' and pid != pg_backend_pid() order by query_start;
  pid  |          query_start          |                query                | wait_event 
-------+-------------------------------+-------------------------------------+------------
 18767 | 2026-04-10 20:38:27.466107+00 | select * from grab_spinlock(12000); | 
 21231 | 2026-04-10 20:38:29.156627+00 | select * from grab_spinlock(12000); | SpinDelay
(2 rows)
```
