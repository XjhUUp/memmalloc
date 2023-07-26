gcc -o pt.out ./test_code/exp_1.c -lpthread
gcc -o tr.out ../tr_malloc/TRmalloc.c ./test_code/exp_1.c -I../tr_malloc/ -DTR_MALLOC -DNTMR -lpthread
gcc -o mem.out  ../src/memmalloc.c  ./test_code/exp_1.c -DMEM_MALLOC   -I../include -lpthread
# gcc -o old_mem.out  ../old_version/src/memmalloc.c  ./test_code/exp_1.c -DMEM_MALLOC   -I../old_version/include -lpthread

i="0"
BASE="10000"
SLOP="10000"
THNUM="1"

while [ $i -lt 15 ]
do
    TIMES=$(($BASE+$i*$SLOP))
    echo -n $TIMES" ">>tr
    # echo -n $TIMES" ">>old_mem
    echo -n $TIMES" ">>pt
    echo -n $TIMES" ">>mem
    ./tr.out $TIMES $THNUM >>tr
    # ./old_mem.out $TIMES $THNUM >>old_mem
    ./pt.out $TIMES $THNUM >>pt
    ./mem.out $TIMES $THNUM >>mem
    echo "" >> tr
    # echo "" >> old_mem
    echo "" >> pt
    echo "" >> mem
    i=$(($i+1))
done
python3 static_analy.py 3
rm pt
rm mem
rm tr
# rm old_mem

 


