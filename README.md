# projectLoRaWAN

This project is an ns-3 module that can be used to perform simulations of a LoRaWAN network, and it is based in the initial version of this code was developed as part of a master's thesis at the University of Padova, under the supervision of Prof. Lorenzo Vangelista, Prof. Michele Zorzi and with the help of Marco Centenaro.

## Getting Started ##

This section is aimed at getting a user to a working state starting with a machine that may never have had ns-3 installed. It covers supported platforms, prerequisites, ways to obtain ns-3, ways to build ns-3, and ways to verify your build and run simple programs.

### Prerequisites ###
```
sudo apt-get update
sudo apt-get -y install gcc g++ python
sudo apt-get -y install gcc g++ python python-dev
sudo apt-get -y install qt4-dev-tools libqt4-dev
sudo apt-get -y install mercurial
sudo apt-get -y install bzr
sudo apt-get -y install cmake libc6-dev libc6-dev-i386 g++-multilib
sudo apt-get -y install gdb valgrind
sudo apt-get -y install gsl-bin libgsl2 libgsl2:i386
sudo apt-get -y install flex bison libfl-dev
sudo apt-get -y install tcpdump
sudo apt-get -y install sqlite sqlite3 libsqlite3-dev
sudo apt-get -y install libxml2 libxml2-dev
sudo apt-get -y install libgtk2.0-0 libgtk2.0-dev
sudo apt-get -y install vtun lxc
sudo apt-get -y install git
git clone https://github.com/heldercs/projectLoRaWAN-NS3
```
