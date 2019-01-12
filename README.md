INSaNe [INSN] 2017-2019 integration/staging tree
=====================================

http://www.insanecoin.com

What is the INSaNe [INSN] Blockchain?
-------------------------------------
*TODO: Update documentation regarding implemented tech as this section is out of date and much progress and upgrades have been made to mentioned sections...*

### Overview
Espers is a blockchain project with the goal of offering secured messaging, websites on the chain and an overall pleasing experience to the user.

### Blockchain Technology
The INSaNe [INSN] Blockchain is an experimental smart contract platform protocol that enables 
instant payments to anyone, anywhere in the world in a private, secure manner. 
INSaNe [INSN] uses peer-to-peer blockchain technology developed by Bitcoin to operate
with no central authority: managing transactions, execution of contracts, and 
issuing money are carried out collectively by the network. INSaNe [INSN] is the name of 
open source software which enables the use of this protocol.

### Custom Difficulty Retarget Algorithm “VRX”
VRX is designed from the ground up to integrate properly with the Velocity parameter enforcement system to ensure users no longer receive orphan blocks.

### Velocity Block Constraint System
Ensuring Insane stays as secure and robust as possible the CryptoCoderz team have implemented what's known as the Velocity block constraint system. This system acts as third and final check for both mined and peer-accepted blocks ensuring that all parameters are strictly enforced.

Specifications and General info
------------------
INSaNe uses 
		
		libsecp256k1,
		libgmp,
		Boost1.68,
		OR Boost1.57,  
		Openssl1.02x,
		OR Openssl1.1x,
		Berkeley DB 6.2.32,
		T5.11 to compile


Block Spacing: 

		5 Minutes
		Stake Minimum Age: 15 Confirmations (PoS-v3) | 30 Minutes (PoS-v2)
		Port: 10255
		RPC Port: 10257


BUILD LINUX
-----------

### Compiling INSaNe daemon on Ubunutu 16.04 LTS Xenial
### Note: guide should be compatible with other Ubuntu versions from 14.04+
### Note: guide updated for compiling with Ubuntu 18.04 LTS Bionic

### Become poweruser
```
sudo -i
```

### Dependencies install
```
cd ~; sudo apt-get install ntp git build-essential libssl1.0-dev libdb-dev libdb++-dev libboost-all-dev libqrencode-dev libcurl4-openssl-dev curl libzip-dev; apt-get update; apt-get upgrade; apt-get install git make automake build-essential libboost-all-dev; apt-get install yasm binutils libcurl4-openssl-dev openssl libssl1.0-dev; sudo apt-get install libgmp-dev;
```

### Dependencies build and link
```
cd ~; wget http://download.oracle.com/berkeley-db/db-6.2.32.NC.tar.gz; tar zxf db-6.2.32.NC.tar.gz; cd db-6.2.32.NC/build_unix; ../dist/configure --enable-cxx; make; sudo make install; sudo ln -s /usr/local/BerkeleyDB.6.2/lib/libdb-6.2.so /usr/lib/libdb-6.2.so; sudo ln -s /usr/local/BerkeleyDB.6.2/lib/libdb_cxx-6.2.so /usr/lib/libdb_cxx-6.2.so; export BDB_INCLUDE_PATH="/usr/local/BerkeleyDB.6.2/include"; export BDB_LIB_PATH="/usr/local/BerkeleyDB.6.2/lib"
```

### Personal upload EXAMPLE
```
cd ~; sudo cp -r /home/ftpuser/ftp/files/INSN-clean/. ~/INSN
```

### GitHub pull RECOMMENDED
```
cd ~; git clone https://github.com/CryptoCoderz/INSN
```

### Build INSN daemon
```
cd ~/INSN/src; chmod a+x obj; chmod a+x leveldb/build_detect_platform; chmod a+x secp256k1; chmod a+x leveldb; chmod a+x ~/INSN/src; chmod a+x ~/INSN; make -f makefile.unix USE_UPNP=-; cd ~; cp ~/INSN/src/INSaNed /usr/local/bin;
```

### Create config file for daemon
```
cd ~; sudo ufw allow 10255/tcp; sudo ufw allow 10257/tcp; sudo mkdir ~/.INSN; cat << "CONFIG" >> ~/.INSN/INSaNe.conf
listen=1
server=1
daemon=1
testnet=0
rpcuser=insaneuser
rpcpassword=SomeCrazyVeryVerySecurePasswordHere
rpcport=10257
port=10255
rpcconnect=127.0.0.1
rpcallowip=127.0.0.1
addnode=104.207.156.13:10255
addnode=139.99.41.157:10255
addnode=140.82.34.177:10255
addnode=149.28.141.100:10255
addnode=150.101.218.46:10255
addnode=150.101.218.47:10255
addnode=198.50.180.192:10255
addnode=198.58.117.101:10255
addnode=199.247.27.137:10255
addnode=207.148.6.197:10255
addnode=212.237.9.227:10255
addnode=45.76.30.175:10255
addnode=45.77.231.160:10255
addnode=77.81.230.78:10255
addnode=80.211.142.194:10255
addnode=80.240.31.212:10255
addnode=82.162.58.186:10255
addnode=95.216.86.224:10255
addnode=95.216.86.225:10255
addnode=181.223.204.137
addnode=107.191.44.183
addnode=139.99.130.60
addnode=139.99.153.9
addnode=159.203.107.73
addnode=198.50.180.198
addnode=198.50.180.207
addnode=45.32.150.182
addnode=104.156.252.216
addnode=104.207.144.242
addnode=104.207.150.76
addnode=104.238.191.194
addnode=108.61.211.69
addnode=165.227.160.144
addnode=207.246.123.32
addnode=45.32.65.126
addnode=45.63.97.94
addnode=45.76.252.172
addnode=62.151.176.140
addnode=70.35.202.246
addnode=74.208.156.184
CONFIG
chmod 700 ~/.INSN/INSaNe.conf; chmod 700 ~/.INSN; ls -la ~/.INSN
```

### Run INSN daemon
```
cd ~; INSaNed; INSaNed getinfo
```

### Troubleshooting
### for basic troubleshooting run the following commands when compiling:
### this is for minupnpc errors compiling

```
make clean -f makefile.unix USE_UPNP=-
make -f makefile.unix USE_UPNP=-
```

License
-------

INSaNe [INSN] is released under the terms of the MIT license. See [COPYING](COPYING) for more
information or see https://opensource.org/licenses/MIT.

Development Process
-------------------

The `master` branch is regularly built and tested, but is not guaranteed to be
completely stable. [Tags](https://github.com/CryptoCoderz/INSN/tags) are created
regularly to indicate new official, stable release versions of INSaNe [INSN].

The contribution workflow is described in [CONTRIBUTING.md](CONTRIBUTING.md).

The developer [mailing list](https://lists.linuxfoundation.org/mailman/listinfo/bitcoin-dev)
should be used to discuss complicated or controversial changes before working
on a patch set.

Developer Discord can be found at https://discord.gg/YjN94vp .

Testing
-------

Testing and code review is the bottleneck for development; we get more pull
requests than we can review and test on short notice. Please be patient and help out by testing
other people's pull requests, and remember this is a security-critical project where any mistake might cost people
lots of money.

### Automated Testing

Developers are strongly encouraged to write [unit tests](/doc/unit-tests.md) for new code, and to
submit new unit tests for old code. Unit tests can be compiled and run
(assuming they weren't disabled in configure) with: `make check`

There are also [regression and integration tests](/qa) of the RPC interface, written
in Python, that are run automatically on the build server.

### Manual Quality Assurance (QA) Testing

Changes should be tested by somebody other than the developer who wrote the
code. This is especially important for large or high-risk changes. It is useful
to add a test plan to the pull request description if testing the changes is
not straightforward.
