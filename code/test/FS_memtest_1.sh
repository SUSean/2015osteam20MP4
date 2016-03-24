make clean
make
# remove all the reports before running
rm *.rpt
# run nachos with valgrind for memleak check
echo "running 1 of 4 ..."
valgrind ../build.linux/nachos -f >/dev/null 2>1.rpt
echo "running 2 of 4 ..."
valgrind ../build.linux/nachos -cp FS_test1 /FS_test1 >/dev/null 2>2.rpt
echo "running 3 of 4 ..."
valgrind ../build.linux/nachos -e /FS_test1 >/dev/null 2>3.rpt
echo "running 4 of 4 ..."
valgrind ../build.linux/nachos -p /file1 >/dev/null 2>4.rpt
# fetch blocks lost info from report
BLOCK_LOST=0
for i in 1 2 3 4
do
	A1=$(cat $i.rpt | grep 'definitely lost' | awk '{print $7}' | sed 's/,//g')
	A2=$(cat $i.rpt | grep 'indirectly lost' | awk '{print $7}' | sed 's/,//g')
	A3=$(cat $i.rpt | grep 'possibly lost' | awk '{print $7}' | sed 's/,//g')
	echo $i 'lost:' $A1 $A2 $A3 'blocks'
	BLOCK_LOST=$((BLOCK_LOST + A1 + A2 + A3))
done
echo 'total lost:' $BLOCK_LOST 'blocks'
