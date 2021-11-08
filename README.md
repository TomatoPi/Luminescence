# Optopoulpe
Des lumières de partout

cat pipes/ctrl2ard | ./arduino-bridge /dev/ttyACM0 1> pipes/ard2ctrl
cat pipes/ctrl2apc | ./apc40-mapping pipes/j2apc pipes/apc2j 1> pipes/apc2ctrl
cat pipes/apc2ctrl | ./controller save.txt pipes/ard2ctrl pipes/ctrl2ard 1> pipes/ctrl2apc
cat pipes/apc2j | ./jackproxy-from-stdin
./jackproxy-to-stdout 1> pipes/j2apc