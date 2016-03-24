# remove all the reports before running
rm *.rpt
# run nachos with valgrind for memleak check
echo "running 1 of 6 ..."
valgrind ../build.linux/nachos -f >/dev/null 2>1.rpt
echo "running 2 of 6 ..."
valgrind ../build.linux/nachos -cp num_100.txt /100 >/dev/null 2>2.rpt
echo "running 3 of 6 ..."
valgrind ../build.linux/nachos -cp num_1000.txt /1000 >/dev/null 2>3.rpt
echo "running 4 of 6 ..."
valgrind ../build.linux/nachos -p /1000 >/dev/null 2>4.rpt
echo "running 5 of 6 ..."
valgrind ../build.linux/nachos -p /100 >/dev/null 2>5.rpt
echo "running 6 of 6 ..."
valgrind ../build.linux/nachos -l / >/dev/null 2>6.rpt
 # fetch blocks lost info from report
BLOCK_LOST=0
for i in 1 2 3 4 5 6
do
	A1=$(cat $i.rpt | grep 'definitely lost' | awk '{print $7}' | sed 's/,//g')
	A2=$(cat $i.rpt | grep 'indirectly lost' | awk '{print $7}' | sed 's/,//g')
	A3=$(cat $i.rpt | grep 'possibly lost' | awk '{print $7}' | sed 's/,//g')
	echo $i 'lost:' $A1 $A2 $A3 'blocks'
	BLOCK_LOST=$((BLOCK_LOST + A1 + A2 + A3))
done
echo 'total lost:' $BLOCK_LOST 'blocks'
