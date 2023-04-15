gcc -o pt.out ./test_code/exp_8.c -lpthread
gcc -o tr.out ../tr_malloc/TRmalloc.c ./test_code/exp_8.c -I../tr_malloc/ -DTR_MALLOC -DNTMR -lpthread
gcc -o mem.out  ../src/memmalloc.c  ./test_code/exp_8.c -DMEM_MALLOC -I../include -lpthread


i="1"
BASE="10000"
SLOP="10000"
THNUM="20"

while [ $i -lt 17 ]
do
    TIMES=$(($BASE+$i*$SLOP))
    echo -n $(($i*64))" ">>pt
    echo -n $(($i*64))" ">>tr
    echo -n $(($i*64))" ">>mem
    ./pt.out $(($i*64)) $THNUM >>pt
    ./tr.out $(($i*64)) $THNUM >>tr
    ./mem.out $(($i*64)) $THNUM >>mem
    echo "" >> pt
    echo "" >> tr
    echo "" >> mem
    i=$(($i+1))
done
python3 static_analy.py 2
rm pt
rm tr
rm mem

 


