gcc -o pt.out ./test_code/exp_3.c -lpthread
gcc -o tr.out ../tr_malloc/TRmalloc.c ./test_code/exp_3.c -I../tr_malloc/ -DTR_MALLOC -DNTMR -lpthread
gcc -o mem.out  ../src/memmalloc.c  ./test_code/exp_3.c -DMEM_MALLOC   -I../include -lpthread


i="0"
BASE="10000"
SLOP="10000"
THNUM="1"

while [ $i -lt 15 ]
do
    TIMES=$(($BASE+$i*$SLOP))
    echo -n $TIMES" ">>pt
    echo -n $TIMES" ">>tr
    echo -n $TIMES" ">>mem
   ./pt.out $TIMES $THNUM >>pt
   ./tr.out $TIMES $THNUM >>tr
    ./mem.out $TIMES $THNUM >>mem
    echo "" >> pt
    echo "" >> tr
    echo "" >> mem
    i=$(($i+1))
done
python3 static_analy.py 5
rm tr
rm pt
rm mem

 


