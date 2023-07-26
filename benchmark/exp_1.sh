gcc -o pt.out ./test_code/exp_8.c -lpthread
gcc -o tr.out ../tr_malloc/TRmalloc.c ./test_code/exp_8.c -I../tr_malloc/ -DTR_MALLOC -DNTMR -lpthread
gcc -o mem.out  ../src/memmalloc.c  ./test_code/exp_8.c -DMEM_MALLOC  -I../include -lpthread
# gcc -o old_mem.out  ../old_version/src/memmalloc.c  ./test_code/exp_2.c -DMEM_MALLOC  -I../old_version/include -lpthread

i="1"
BASE="10000"
SLOP="10000"
THNUM="1"

while [ $i -lt 17 ]
do
    TIMES=$(($BASE+$i*$SLOP))
    # echo -n $TIMES" ">>old_mem
    echo -n $(($i*64))" ">>pt
    echo -n $(($i*64))" ">>tr
    echo -n $(($i*64))" ">>mem
    # ./old_mem.out $TIMES $THNUM >>old_mem
    ./pt.out $(($i*64)) $THNUM >>pt
    ./tr.out $(($i*64)) $THNUM >>tr
    ./mem.out $(($i*64)) $THNUM >>mem
    # echo "" >> old_mem
    echo "" >> pt
    echo "" >> tr
    echo "" >> mem
    i=$(($i+1))
done
python3 static_analy.py 1
rm pt
rm tr
rm mem
# rm old_mem

 


