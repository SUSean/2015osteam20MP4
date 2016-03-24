../build.linux/nachos -f
../build.linux/nachos -mkdir /z0
../build.linux/nachos -mkdir /z0/y0
../build.linux/nachos -mkdir /z0/y0/x0
../build.linux/nachos -cp nachos.html /z0/y0/x0/nachos
echo "======section 1======"
../build.linux/nachos -p /z0/y0/x0/nachos
echo "====================="
echo "======section 2======"
../build.linux/nachos -l /z0/y0/x0
echo "====================="
../build.linux/nachos -r /z0/y0/x0/nachos
echo "======seciton 3======"
../build.linux/nachos -l /z0/y0/x0
echo "====================="
