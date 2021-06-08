#!/bin/bash

echo 3600 >/proc/sys/net/ipv4/neigh/default/gc_stale_time
echo 4 >/proc/sys/net/ipv4/tcp_retries2
