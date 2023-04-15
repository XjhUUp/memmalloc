gcc -o pt.out ./test_code/exp_4.c -lpthread
gcc -o nsat.out  ../src/memmalloc.c  ./test_code/exp_4.c -DMEM_MALLOC   -I../include -lpthread
gcc -o seu.out  ../src/memmalloc.c  ./test_code/exp_4.c -DMEM_MALLOC -DSEU  -I../include -lpthread
gcc -o tmr.out  ../src/memmalloc.c  ./test_code/exp_4.c -DMEM_MALLOC -DTMR  -I../include -lpthread

i="0"
BASE="10000"
SLOP="10000"
THNUM="1"

while [ $i -lt 15 ]
do
    TIMES=$(($BASE+$i*$SLOP))
    echo -n $TIMES" " >> pt
    echo -n $TIMES" " >> nsat
    echo -n $TIMES" " >> seu
    echo -n $TIMES" " >> tmr
    ./pt.out $TIMES $THNUM >> pt
    ./nsat.out $TIMES $THNUM >> nsat
    ./seu.out $TIMES $THNUM >> seu
    ./tmr.out $TIMES $THNUM >> tmr
    echo "" >> pt
    echo "" >> nsat
    echo "" >> seu
    echo "" >> tmr

    i=$(($i+1))
done
python3 static_analy.py 9
rm pt
rm nsat
rm seu
rm tmr
