valgrind ../build.linux/nachos -f
valgrind ../build.linux/nachos -mkdir /t0
valgrind ../build.linux/nachos -mkdir /t1
valgrind ../build.linux/nachos -mkdir /t2
valgrind ../build.linux/nachos -cp num_100.txt /t0/f1
valgrind ../build.linux/nachos -mkdir /t0/aa
valgrind ../build.linux/nachos -mkdir /t0/bb
valgrind ../build.linux/nachos -mkdir /t0/cc
valgrind ../build.linux/nachos -cp num_100.txt /t0/bb/f1
valgrind ../build.linux/nachos -cp num_100.txt /t0/bb/f2
valgrind ../build.linux/nachos -cp num_100.txt /t0/bb/f3
valgrind ../build.linux/nachos -cp num_100.txt /t0/bb/f4
valgrind ../build.linux/nachos -l /
echo "========================================="
valgrind ../build.linux/nachos -l /t0
echo "========================================="
valgrind ../build.linux/nachos -l /t0/bb
echo "========================================="
valgrind ../build.linux/nachos -p /t0/f1
echo "========================================="
valgrind ../build.linux/nachos -p /t0/bb/f3
