#!/bin/sh

D_RAW=https://raw.githubusercontent.com/notracking/hosts-blocklists/master/domains.txt
H_RAW=https://raw.githubusercontent.com/notracking/hosts-blocklists/master/hostnames.txt
DOMAINS=domains.txt
HOSTS=hostnames.txt
BLIST=blist

curl ${D_RAW} > ${DOMAINS}
curl ${H_RAW} > ${HOSTS}

grep "0\.0\.0\.0" ${DOMAINS} | cut -d/ -f2 > ${BLIST}
grep "0\.0\.0\.0" ${HOSTS} | cut -d' ' -f2 >> ${BLIST}

rm ${DOMAINS} ${HOSTS}
