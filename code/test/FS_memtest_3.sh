# remove all the reports before running
rm *.rpt
# run nachos with valgrind for memleak check
echo "running 1 of 11 ..."
valgrind ../build.linux/nachos -f >/dev/null 2>1.rpt
echo "running 2 of 11 ..."
valgrind ../build.linux/nachos -mkdir /t0 >/dev/null 2>2.rpt
echo "running 3 of 11 ..."
valgrind ../build.linux/nachos -mkdir /t1 >/dev/null 2>3.rpt
echo "running 4 of 11 ..."
valgrind ../build.linux/nachos -cp num_100.txt /t0/f1 >/dev/null 2>4.rpt
echo "running 5 of 11 ..."
valgrind ../build.linux/nachos -mkdir /t0/aa >/dev/null 2>5.rpt
echo "running 6 of 11 ..."
valgrind ../build.linux/nachos -mkdir /t0/bb >/dev/null 2>6.rpt
echo "running 7 of 11 ..."
valgrind ../build.linux/nachos -cp num_100.txt /t0/bb/f3 >/dev/null 2>7.rpt
echo "running 8 of 11 ..."
valgrind ../build.linux/nachos -l /t0 >/dev/null 2>8.rpt
echo "running 9 of 11 ..."
valgrind ../build.linux/nachos -l /t0/bb >/dev/null 2>9.rpt
echo "running 10 of 11 ..."
valgrind ../build.linux/nachos -p /t0/f1 >/dev/null 2>10.rpt
echo "running 11 of 11 ..."
valgrind ../build.linux/nachos -r /t0/bb/f3 >/dev/null 2>11.rpt
# fetch blocks lost info from report
BLOCK_LOST=0
for (( i=1; i<=11; i++))
do
        A1=$(cat $i.rpt | grep 'definitely lost' | awk '{print $7}' | sed 's/,//g')
        A2=$(cat $i.rpt | grep 'indirectly lost' | awk '{print $7}' | sed 's/,//g')
        A3=$(cat $i.rpt | grep 'possibly lost' | awk '{print $7}' | sed 's/,//g')
        echo $i 'lost:' $A1 $A2 $A3 'blocks'
        BLOCK_LOST=$((BLOCK_LOST + A1 + A2 + A3))
done
echo 'total lost:' $BLOCK_LOST 'blocks'

