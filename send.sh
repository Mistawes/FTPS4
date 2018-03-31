# golden
# change the ip for different networks...

socat -u FILE:payload.bin TCP:192.168.0.22:9020
sleep 0.25
socat -u FILE:kpayload.elf TCP:192.168.0.22:9023
