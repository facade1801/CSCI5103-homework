# make clean
make
sudo ./scull_unload
sudo ./scull_load
sudo chmod 777 /dev/scullbuffer
# sudo ./producer 5 red; sudo ./producer 5 blue; sudo ./consumer 5 red; sudo ./consumer 5 blue
sudo ./producer 5 red; sudo ./consumer 5 red
sudo dmesg -c