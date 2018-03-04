// Copyright (c) 2016-2018 The CryptoCoderz Team / Espers
// Copyright (c) 2018 The CryptoCoderz Team / INSaNE project
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "blockparams.h"
#include "chainparams.h"
#include "checkpoints.h"
#include "db.h"
#include "init.h"
#include "kernel.h"
#include "net.h"
#include "txdb.h"
#include "velocity.h"
#include "main.h"

#include <boost/random/mersenne_twister.hpp>
#include <boost/random/uniform_int_distribution.hpp>

using namespace std;
using namespace boost;

//////////////////////////////////////////////////////////////////////////////
//
// Standard Global Values
//

//
// Section defines global values for retarget logic
//

double VLF1 = 0;
double VLF2 = 0;
double VLF3 = 0;
double VLF4 = 0;
double VLF5 = 0;
double VLFtmp = 0;
double VRFsm1 = 1;
double VRFdw1 = 0.75;
double VRFdw2 = 0.5;
double VRFup1 = 1.25;
double VRFup2 = 1.5;
double VRFup3 = 2;
double TerminalAverage = 0;
double TerminalFactor = 10000;
CBigNum newBN = 0;
CBigNum oldBN = 0;
int64_t VLrate1 = 0;
int64_t VLrate2 = 0;
int64_t VLrate3 = 0;
int64_t VLrate4 = 0;
int64_t VLrate5 = 0;
int64_t VLRtemp = 0;
int64_t DSrateNRM = BLOCK_SPACING;
int64_t DSrateMAX = BLOCK_SPACING_MAX;
int64_t FRrateDWN = DSrateNRM - 60;
int64_t FRrateFLR = DSrateNRM - 80;
int64_t FRrateCLNG = DSrateMAX * 3;
int64_t difficultyfactor = 0;
int64_t AverageDivisor = 5;
int64_t scanheight = 6;
int64_t scanblocks = 1;
int64_t scantime_1 = 0;
int64_t scantime_2 = 0;
int64_t prevPoW = 0; // hybrid value
int64_t prevPoS = 0; // hybrid value
const CBlockIndex* pindexPrev = 0;
const CBlockIndex* BlockVelocityType = 0;
CBigNum bnOld;
CBigNum bnNew;
unsigned int retarget = DIFF_VRX; // Default with VRX
uint64_t cntTime = 0;
uint64_t prvTime = 0;
uint64_t difTimePoS = 0;
uint64_t difTimePoW = 0;


//////////////////////////////////////////////////////////////////////////////
//
// Debug section
//

//
// Debug log printing
//

void VRXswngPoSdebug()
{
    // Print for debugging
    LogPrintf("Previously discovered PoS block: %u: \n",prvTime);
    LogPrintf("Current block-time: %u: \n",cntTime);
    LogPrintf("Time since last PoS block: %u: \n",difTimePoS);
    if(difTimePoS > 1 * 60 * 60) { TerminalAverage /= 2; LogPrintf("diffTimePoS is greater than 1 Hours: %u \n",cntTime);}
    if(difTimePoS > 2 * 60 * 60) { TerminalAverage /= 2; LogPrintf("diffTimePoS is greater than 2 Hours: %u \n",cntTime);}
    if(difTimePoS > 3 * 60 * 60) { TerminalAverage /= 2; LogPrintf("diffTimePoS is greater than 3 Hours: %u \n",cntTime);}
    if(difTimePoS > 4 * 60 * 60) { TerminalAverage /= 2; LogPrintf("diffTimePoS is greater than 4 Hours: %u \n",cntTime);}
    return;
}

void VRXswngPoWdebug()
{
    // Print for debugging
    LogPrintf("Previously discovered PoW block: %u: \n",prvTime);
    LogPrintf("Current block-time: %u: \n",cntTime);
    LogPrintf("Time since last PoW block: %u: \n",difTimePoW);
    if(difTimePoW > 1 * 60 * 60) { TerminalAverage /= 2; LogPrintf("diffTimePoW is greater than 1 Hours: %u \n",cntTime);}
    if(difTimePoW > 2 * 60 * 60) { TerminalAverage /= 2; LogPrintf("diffTimePoW is greater than 2 Hours: %u \n",cntTime);}
    if(difTimePoW > 3 * 60 * 60) { TerminalAverage /= 2; LogPrintf("diffTimePoW is greater than 3 Hours: %u \n",cntTime);}
    if(difTimePoW > 4 * 60 * 60) { TerminalAverage /= 2; LogPrintf("diffTimePoW is greater than 4 Hours: %u \n",cntTime);}
    return;
}

void VRXdebug()
{
    // Print for debugging
    LogPrintf("Terminal-Velocity 1st spacing: %u: \n",VLrate1);
    LogPrintf("Terminal-Velocity 2nd spacing: %u: \n",VLrate2);
    LogPrintf("Terminal-Velocity 3rd spacing: %u: \n",VLrate3);
    LogPrintf("Terminal-Velocity 4th spacing: %u: \n",VLrate4);
    LogPrintf("Terminal-Velocity 5th spacing: %u: \n",VLrate5);
    LogPrintf("Desired normal spacing: %u: \n",DSrateNRM);
    LogPrintf("Desired maximum spacing: %u: \n",DSrateMAX);
    LogPrintf("Terminal-Velocity 1st multiplier set to: %f: \n",VLF1);
    LogPrintf("Terminal-Velocity 2nd multiplier set to: %f: \n",VLF2);
    LogPrintf("Terminal-Velocity 3rd multiplier set to: %f: \n",VLF3);
    LogPrintf("Terminal-Velocity 4th multiplier set to: %f: \n",VLF4);
    LogPrintf("Terminal-Velocity 5th multiplier set to: %f: \n",VLF5);
    LogPrintf("Terminal-Velocity averaged a final multiplier of: %f: \n",TerminalAverage);
    LogPrintf("Prior Terminal-Velocity: %u\n", oldBN);
    LogPrintf("New Terminal-Velocity:  %u\n", newBN);
    return;
}

void GNTdebug()
{
    // Print for debugging
    // Retarget using DGW-v3
    if (retarget != DIFF_VRX)
    {
        // debug info for testing
        LogPrintf("GetNextTargetRequired() : Invalid retarget selection, using default \n");
        return;
    }

    // Retarget using Terminal-Velocity
    // debug info for testing
    LogPrintf("Terminal-Velocity retarget selected \n");
    LogPrintf("Espers retargetted using: Terminal-Velocity difficulty curve \n");
    return;
}


//////////////////////////////////////////////////////////////////////////////
//
// Difficulty retarget (current section)
//

//
// This is VRX revised implementation
//
// Terminal-Velocity-RateX, v10-Beta-R7, written by Jonathan Dan Zaretsky - cryptocoderz@gmail.com
void VRX_BaseEngine(const CBlockIndex* pindexLast, bool fProofOfStake)
{
       // Set base values
       VLF1 = 0;
       VLF2 = 0;
       VLF3 = 0;
       VLF4 = 0;
       VLF5 = 0;
       VLFtmp = 0;
       TerminalAverage = 0;
       TerminalFactor = 10000;
       VLrate1 = 0;
       VLrate2 = 0;
       VLrate3 = 0;
       VLrate4 = 0;
       VLrate5 = 0;
       VLRtemp = 0;
       difficultyfactor = 0;
       scanblocks = 1;
       scantime_1 = 0;
       scantime_2 = pindexLast->GetBlockTime();
       prevPoW = 0; // hybrid value
       prevPoS = 0; // hybrid value
       // Set prev blocks...
       pindexPrev = pindexLast;
       // ...and deduce spacing
       while(scanblocks < scanheight)
       {
           scantime_1 = scantime_2;
           pindexPrev = pindexPrev->pprev;
           scantime_2 = pindexPrev->GetBlockTime();
           // Set standard values
           if(scanblocks > 0){
               if     (scanblocks < scanheight-4){ VLrate1 = (scantime_1 - scantime_2); VLRtemp = VLrate1; }
               else if(scanblocks < scanheight-3){ VLrate2 = (scantime_1 - scantime_2); VLRtemp = VLrate2; }
               else if(scanblocks < scanheight-2){ VLrate3 = (scantime_1 - scantime_2); VLRtemp = VLrate3; }
               else if(scanblocks < scanheight-1){ VLrate4 = (scantime_1 - scantime_2); VLRtemp = VLrate4; }
               else if(scanblocks < scanheight-0){ VLrate5 = (scantime_1 - scantime_2); VLRtemp = VLrate5; }
           }
           // Round factoring
           if(VLRtemp >= DSrateNRM){ VLFtmp = VRFsm1;
               if(VLRtemp > DSrateMAX){ VLFtmp = VRFdw1;
                   if(VLRtemp > FRrateCLNG){ VLFtmp = VRFdw2; }
               }
           }
           else if(VLRtemp < DSrateNRM){ VLFtmp = VRFup1;
               if(VLRtemp < FRrateDWN){ VLFtmp = VRFup2;
                   if(VLRtemp < FRrateFLR){ VLFtmp = VRFup3; }
               }
           }
           // Record factoring
           if      (scanblocks < scanheight-4) VLF1 = VLFtmp;
           else if (scanblocks < scanheight-3) VLF2 = VLFtmp;
           else if (scanblocks < scanheight-2) VLF3 = VLFtmp;
           else if (scanblocks < scanheight-1) VLF4 = VLFtmp;
           else if (scanblocks < scanheight-0) VLF5 = VLFtmp;
           // Log hybrid block type
           //
           // v1.0
           if(pindexBest->GetBlockTime() < 1520146000) // OFF (NOT TOGGLED)
           {
                if     (fProofOfStake) prevPoS ++;
                else if(!fProofOfStake) prevPoW ++;
           }
           // v1.1
           if(pindexBest->GetBlockTime() > 1520146000) // OFF (NOT TOGGLED)
           {
               if(pindexPrev->IsProofOfStake())
               {
                   prevPoS ++;
               }
               else if(pindexPrev->IsProofOfWork())
               {
                   prevPoW ++;
               }
           }

           // move up per scan round
           scanblocks ++;
       }
       // Final mathematics
       TerminalAverage = (VLF1 + VLF2 + VLF3 + VLF4 + VLF5) / AverageDivisor;
       return;
}

void VRX_ThreadCurve(const CBlockIndex* pindexLast, bool fProofOfStake)
{
    // Run VRX engine
    VRX_BaseEngine(pindexLast, fProofOfStake);

    //
    // Skew for less selected block type
    //

    // Version 1.0
    //
    int64_t nNow = nBestHeight; int64_t nThen = 1493596800; // Toggle skew system fork - Mon, 01 May 2017 00:00:00 GMT
    if(nNow > nThen){if(prevPoW < prevPoS && !fProofOfStake){if((prevPoS-prevPoW) > 3) TerminalAverage /= 3;}
    else if(prevPoW > prevPoS && fProofOfStake){if((prevPoW-prevPoS) > 3) TerminalAverage /= 3;}
    if(TerminalAverage < 0.5) TerminalAverage = 0.5;} // limit skew to halving

    // Version 1.1 curve-patch
    //
    if(pindexBest->GetBlockTime() > 1520146000) // OFF (NOT TOGGLED)
    {
        // Define time values
        cntTime = BlockVelocityType->GetBlockTime();
        prvTime = BlockVelocityType->pprev->GetBlockTime();

        if(fProofOfStake)
        {
            difTimePoS = cntTime - prvTime;

            // Debug print toggle
            if(fDebug) VRXswngPoSdebug();
            // Normal Run
            else if(!fDebug)
            {
                if(difTimePoS > 1 * 60 * 60) { TerminalAverage /= 2; }
                if(difTimePoS > 2 * 60 * 60) { TerminalAverage /= 2; }
                if(difTimePoS > 3 * 60 * 60) { TerminalAverage /= 2; }
                if(difTimePoS > 4 * 60 * 60) { TerminalAverage /= 2; }
            }
        }
        else if(!fProofOfStake)
        {
            difTimePoW = cntTime - prvTime;

            // Debug print toggle
            if(fDebug) VRXswngPoWdebug();
            // Normal Run
            else if(!fDebug)
            {
                if(difTimePoW > 1 * 60 * 60) { TerminalAverage /= 2; }
                if(difTimePoW > 2 * 60 * 60) { TerminalAverage /= 2; }
                if(difTimePoW > 3 * 60 * 60) { TerminalAverage /= 2; }
                if(difTimePoW > 4 * 60 * 60) { TerminalAverage /= 2; }
            }
        }
    }
    return;
}

unsigned int VRX_Retarget(const CBlockIndex* pindexLast, bool fProofOfStake)
{
    const CBigNum bnVelocity = fProofOfStake ? Params().ProofOfStakeLimit() : Params().ProofOfWorkLimit();

    // Check for blocks to index | Allowing for initial chain start
    if (pindexLast->nHeight < scanheight+114)
        return bnVelocity.GetCompact(); // can't index prevblock

    // Differentiate PoW/PoS prev block
    BlockVelocityType = GetLastBlockIndex(pindexLast, fProofOfStake);

    // Run VRX threadcurve
    VRX_ThreadCurve(pindexLast, fProofOfStake);

    // Retarget
    TerminalFactor *= TerminalAverage;
    difficultyfactor = TerminalFactor;
    bnOld.SetCompact(BlockVelocityType->nBits);
    bnNew = bnOld / difficultyfactor;
    bnNew *= 10000;

    // Limit
    if (bnNew > bnVelocity)
      bnNew = bnVelocity;

    // Final log
    oldBN = bnOld.GetCompact();
    newBN = bnNew.GetCompact();

    // Debug print toggle
    if(fDebug) VRXdebug();

    // Return difficulty
    return bnNew.GetCompact();
}

//////////////////////////////////////////////////////////////////////////////
//
// Difficulty retarget (function)
//

unsigned int GetNextTargetRequired(const CBlockIndex* pindexLast, bool fProofOfStake)
{
    // Default with VRX
    unsigned int retarget = DIFF_VRX;

    // Check selection
    if (retarget != DIFF_VRX)
    {
        // debug info for testing
        if(fDebug) GNTdebug();
        return VRX_Retarget(pindexLast, fProofOfStake);
    }

    // Retarget using Terminal-Velocity
    // debug info for testing
    if(fDebug) GNTdebug();
    return VRX_Retarget(pindexLast, fProofOfStake);
}

//////////////////////////////////////////////////////////////////////////////
//
// Coin base subsidy
//

int static generateMTRandom(unsigned int s, int range)
{
    random::mt19937 gen(s);
    random::uniform_int_distribution<> dist(0, range);
    return dist(gen);
}

int randreward()
{
    // Superblock calculations
    uint256 prevHash = 0;
    if(pindexBest->pprev)
        prevHash = pindexBest->pprev->GetBlockHash();
    std::string cseed_str = prevHash.ToString().substr(7,7);
    const char* cseed = cseed_str.c_str();
    long seed = hex2long(cseed);
    int rand1 = generateMTRandom(seed, 1000000);
    return rand1;
}

//
// PoW coin base reward
//
int64_t GetProofOfWorkReward(int nHeight, int64_t nFees)
{
    int64_t nSubsidy = nBlockPoWReward;

    if(nHeight > nReservePhaseStart && nHeight < nReservePhaseEnd) {
      nSubsidy = nBlockRewardReserve;
    }
    if(randreward() <= 8000 && nHeight > nReservePhaseEnd) // 8% Chance of superblock
        nSubsidy *= nSuperModifier; // x2
    if(pindexBest->GetBlockTime() > 1520146000) // OFF (NOT TOGGLED)
    {
        nSubsidy = nBlockPoWReward_2; // 5.5

        if(randreward() <= 20000 && nHeight > nReservePhaseEnd) // 20% Chance of superblock
            nSubsidy *= nSuperModifier; // x2
    }

    // hardCap v2.1
    else if(pindexBest->nMoneySupply > MAX_SINGLE_TX)
    {
        LogPrint("MINEOUT", "GetProofOfWorkReward(): create=%s nFees=%d\n", FormatMoney(nFees), nFees);
        return nFees;
    }

    LogPrint("creation", "GetProofOfWorkReward() : create=%s nSubsidy=%d\n", FormatMoney(nSubsidy), nSubsidy);
    return nSubsidy + nFees;
}

//
// PoS coin base reward
//
int64_t GetProofOfStakeReward(const CBlockIndex* pindexPrev, int64_t nCoinAge, int64_t nFees)
{
    int64_t nSubsidy = nCoinAge * COIN_YEAR_REWARD * 33 / (365 * 33 + 8);

    if(randreward() <= 8000) // 8% Chance of superblock
        nSubsidy = nCoinAge * COIN_SPRB_REWARD * 33 / (365 * 33 + 8);
    if(nBestHeight > RWRD_FIX_TOGGLE) // Correct block reward payouts
    {
        nSubsidy = nCoinAge * COIN_YEAR_REWARD_FIXED * 33 / (365 * 33 + 8);
        if(randreward() <= 8000) // 8% Chance of superblock (Fixed)
            nSubsidy = nCoinAge * COIN_SPRB_REWARD_FIXED * 33 / (365 * 33 + 8);
        // Correct subsidy for proper MN allocation
        if(nBestHeight > MN_FIX_TOGGLE)
            nSubsidy = nCoinAge * MN_REWARD_FIXED * 33 / (365 * 33 + 8);
        if(pindexBest->GetBlockTime() > 1520146000) // OFF (NOT TOGGLED)
        {
            nSubsidy = STATIC_POS_REWARD;
            if(randreward() <= 8000) // 8% Chance of superblock
                nSubsidy = (STATIC_POS_REWARD * STATIC_POS_MULTIPLIER) / 100;
        }
    }

    // hardCap v2.1
    else if(pindexBest->nMoneySupply > MAX_SINGLE_TX)
    {
        LogPrint("MINEOUT", "GetProofOfStakeReward(): create=%s nFees=%d\n", FormatMoney(nFees), nFees);
        return nFees;
    }

    LogPrint("creation", "GetProofOfStakeReward(): create=%s nCoinAge=%d\n", FormatMoney(nSubsidy), nCoinAge);
    return nSubsidy + nFees;
}

//
// Masternode coin base reward
//
int64_t GetMasternodePayment(int nHeight, int64_t blockValue)
{
    int64_t ret = 1 * COIN;

    if(pindexBest->GetBlockTime() < 1520146000) // OFF (NOT TOGGLED)
    {
        ret = blockValue * 1/6; // 1/6th

        // Correct MN payout to reflect posted rates
        if(nHeight > MN_FIX_TOGGLE) // ON (TODO: GET FORK TIME)
        {
            ret = blockValue * 5/6; // 5/6th
            if(randreward() <= 8000)
                ret = blockValue * 4/6; // 4/6th
        }
    }
    else if(pindexBest->GetBlockTime() > 1520146000) // OFF (NOT TOGGLED)
    {
        ret = (blockValue * 85) / 100; // 85%
    }

    return ret;
}

//
// DevOps coin base reward
//
int64_t GetDevOpsPayment(int nHeight, int64_t blockValue)
{
    int64_t ret2 = 0;
    ret2 = (blockValue * 5) / 100; // 5%

    return ret2;
}
