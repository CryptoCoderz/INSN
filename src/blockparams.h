// Copyright (c) 2016-2018 The CryptoCoderz Team / Espers
// Copyright (c) 2018 The CryptoCoderz Team / INSaNE project
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.
#ifndef BITCOIN_BLOCKPARAMS_H
#define BITCOIN_BLOCKPARAMS_H

#include "net.h"
#include "core.h"
#include "bignum.h"

#define START_MASTERNODE_PAYMENTS_TESTNET      1495238400  // ON (Sat, 20 May 2017 00:00:00 GMT)
#define START_MASTERNODE_PAYMENTS              1495238400  // ON (Sat, 20 May 2017 00:00:00 GMT)
#define STOP_MASTERNODE_PAYMENTS_TESTNET       9993058800  // OFF (NOT TOGGLED)
#define STOP_MASTERNODE_PAYMENTS               9993058800  // OFF (NOT TOGGLED)

#define START_DEVOPS_PAYMENTS_TESTNET          9993058800  // OFF (NOT TOGGLED)
#define START_DEVOPS_PAYMENTS                  9993058800  // OFF (NOT TOGGLED)
#define STOP_DEVOPS_PAYMENTS_TESTNET           9993058800  // OFF (NOT TOGGLED)
#define STOP_DEVOPS_PAYMENTS                   9993058800  // OFF (NOT TOGGLED)

#define INSTANTX_SIGNATURES_REQUIRED           10
#define INSTANTX_SIGNATURES_TOTAL              15

// Define difficulty retarget algorithms
enum DiffMode {
    DIFF_DEFAULT = 0, // Default to invalid 0
    DIFF_VRX     = 1, // Retarget using Terminal-Velocity-RateX
};

void VRXswngPoSdebug();
void VRXswngPoWdebug();
void VRXdebug();
void GNTdebug();
void VRX_BaseEngine(const CBlockIndex* pindexLast, bool fProofOfStake);
void VRX_ThreadCurve(const CBlockIndex* pindexLast, bool fProofOfStake);
unsigned int VRX_Retarget(const CBlockIndex* pindexLast, bool fProofOfStake);
unsigned int GetNextTargetRequired(const CBlockIndex* pindexLast, bool fProofOfStake);
int64_t GetProofOfWorkReward(int nHeight, int64_t nFees);
int64_t GetProofOfStakeReward(const CBlockIndex* pindexPrev, int64_t nCoinAge, int64_t nFees);
int64_t GetMasternodePayment(int nHeight, int64_t blockValue);
int64_t GetDevOpsPayment(int nHeight, int64_t blockValue);


#endif // BITCOIN_BLOCKPARAMS_H
