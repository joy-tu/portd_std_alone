pkill ^portd$
#./portd/portd -p 1 -f tcp_srv.conf
#./portd/portd -p 1
mv ./src/src ./src/portd
./src/portd
