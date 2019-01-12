// Copyright (c) 2016-2019 The CryptoCoderz Team / Espers
// Copyright (c) 2019 The CryptoCoderz Team / INSaNE project
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.
#ifndef BITCOIN_GENESIS_H
#define BITCOIN_GENESIS_H

#include "bignum.h"

/** Genesis Start Time */
static const unsigned int timeGenesisBlock = 1493596800; // Mon, 01 May 2017 00:00:00 GMT
/** Genesis TestNet Start Time */
static const unsigned int timeTestNetGenesis = 1493596800+30; // Mon, 01 May 2017 00:00:30 GMT
/** Genesis RegNet Start Time */
static const unsigned int timeRegNetGenesis = 1493596800+90; // Mon, 01 May 2017 00:00:90 GMT
/** Genesis Nonce Mainnet*/
static const unsigned int nNonceMain = 192744;
/** Genesis Nonce Testnet */
static const unsigned int nNonceTest = 5925;
/** Genesis Nonce Regnet */
static const unsigned int nNonceReg = 8;
/** Main Net Genesis Block */
static const uint256 nGenesisBlock("0x00001f66cb3ba8f5776cb750d621cb3390200580cc39f076b3f61efcf191fba0");
/** Test Net Genesis Block */
static const uint256 hashTestNetGenesisBlock("0x0000ae1d0aaeda3c5554fc4d5192c481d002174e33985bb8c855edd899fd0346");
/** Reg Net Genesis Block */
static const uint256 hashRegNetGenesisBlock("0xb772ef430a34e04f015ab7a4e4fbe2e882794a83b1dc0056573d74880649d073");
/** Genesis Merkleroot */
static const uint256 nGenesisMerkle("0xe7dba9a3b6015db6a7e3184106c0f813f525b9d4528f36d6f4da0927c9bf0a5f");

#endif // BITCOIN_GENESIS_H