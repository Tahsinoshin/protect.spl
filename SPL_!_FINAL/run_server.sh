make server
clear
echo enter $(hostname -I | awk '{print $1}') as server ip
echo
echo
./server 1200
