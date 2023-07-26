gcc -o pt.out ./test_code/exp_5.c -lpthread
gcc -o tr.out ../tr_malloc/TRmalloc.c ./test_code/exp_5.c -I../tr_malloc/ -DTR_MALLOC -DNTMR -lpthread
gcc -o mem.out  ../src/memmalloc.c  ./test_code/exp_5.c -DMEM_MALLOC -DNSEU  -I../include -lpthread


i="0"
BASE="10000"
SLOP="10000"
THNUM="20"

while [ $i -lt 15 ]
do
    TIMES=$(($BASE+$i*$SLOP))
    ./memusg.sh ./tr.out $TIMES $THNUM >> tr
    echo -n $TIMES" " >> tr
    ./memusg.sh ./pt.out $TIMES $THNUM >> pt
    echo -n $TIMES" " >> pt
    ./memusg.sh ./mem.out $TIMES $THNUM >> mem
    echo -n $TIMES" " >> mem

    i=$(($i+1))
done
python3 static_analy.py 12
rm tr
rm pt
rm mem