#!/bin/sh

DOMAINS=domains.txt
HOSTS=hostnames.txt
BLIST=blist

grep "0\.0\.0\.0" ${DOMAINS} | cut -d/ -f2 > ${BLIST}
grep "0\.0\.0\.0" ${HOSTS} | cut -d' ' -f2 >> ${BLIST}
