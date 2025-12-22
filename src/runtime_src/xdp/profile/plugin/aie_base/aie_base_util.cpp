// SPDX-License-Identifier: Apache-2.0
// Copyright (C) 2022-2025 Advanced Micro Devices, Inc. All rights reserved

#define XDP_PLUGIN_SOURCE

#include "xdp/profile/plugin/aie_base/aie_base_util.h"
#include "xdp/profile/database/static_info/aie_util.h"
#include "xdp/profile/database/static_info/aie_constructs.h"
#include "xaiefal/xaiefal.hpp"

namespace xdp::aie {

  /****************************************************************************
   * Build 2-channel broadcast network for timer synchronization
   ***************************************************************************/
  void build2ChannelBroadcastNetwork(XAie_DevInst* aieDevInst, uint8_t broadcastId1,
                                     uint8_t broadcastId2, XAie_Events event,
                                     uint8_t startCol, uint8_t numCols, uint8_t numRows,
                                     uint8_t rowOffset)
  {
    XAie_Events bcastEvent2_PL = (XAie_Events)(XAIE_EVENT_BROADCAST_A_0_PL + broadcastId2);
    XAie_EventBroadcast(aieDevInst, XAie_TileLoc(startCol, 0), XAIE_PL_MOD, broadcastId2, event);

    for(uint8_t col = startCol; col < startCol + numCols; col++) {
      for(uint8_t row = 0; row < numRows; row++) {
        module_type tileType = getModuleType(row, rowOffset);
        auto loc = XAie_TileLoc(col, row);

        if(tileType == module_type::shim) {
          // first channel is only used to send north
          if(col == startCol) {
            XAie_EventBroadcast(aieDevInst, loc, XAIE_PL_MOD, broadcastId1, event);
          }
          else {
            XAie_EventBroadcast(aieDevInst, loc, XAIE_PL_MOD, broadcastId1, bcastEvent2_PL);
          }
          if(row != numRows-1) {
            XAie_EventBroadcastBlockDir(aieDevInst, loc, XAIE_PL_MOD, XAIE_EVENT_SWITCH_A, broadcastId1, XAIE_EVENT_BROADCAST_SOUTH | XAIE_EVENT_BROADCAST_WEST | XAIE_EVENT_BROADCAST_EAST);
          }
          else {
            XAie_EventBroadcastBlockDir(aieDevInst, loc, XAIE_PL_MOD, XAIE_EVENT_SWITCH_A, broadcastId1, XAIE_EVENT_BROADCAST_SOUTH | XAIE_EVENT_BROADCAST_WEST | XAIE_EVENT_BROADCAST_EAST | XAIE_EVENT_BROADCAST_NORTH);
          }

          // second channel is only used to send east
          XAie_EventBroadcastBlockDir(aieDevInst, loc, XAIE_PL_MOD, XAIE_EVENT_SWITCH_A, broadcastId2, XAIE_EVENT_BROADCAST_SOUTH | XAIE_EVENT_BROADCAST_WEST | XAIE_EVENT_BROADCAST_NORTH);

          if(col != startCol + numCols - 1) {
            XAie_EventBroadcastBlockDir(aieDevInst, loc, XAIE_PL_MOD, XAIE_EVENT_SWITCH_B, broadcastId2, XAIE_EVENT_BROADCAST_SOUTH | XAIE_EVENT_BROADCAST_WEST | XAIE_EVENT_BROADCAST_NORTH);
          }
          else {
            XAie_EventBroadcastBlockDir(aieDevInst, loc, XAIE_PL_MOD, XAIE_EVENT_SWITCH_B, broadcastId2, XAIE_EVENT_BROADCAST_SOUTH | XAIE_EVENT_BROADCAST_WEST | XAIE_EVENT_BROADCAST_NORTH | XAIE_EVENT_BROADCAST_EAST);
          }
        }
        else if(tileType == module_type::mem_tile) {
          if(row != numRows-1) {
            XAie_EventBroadcastBlockDir(aieDevInst, loc, XAIE_MEM_MOD, XAIE_EVENT_SWITCH_A, broadcastId1, XAIE_EVENT_BROADCAST_SOUTH | XAIE_EVENT_BROADCAST_WEST | XAIE_EVENT_BROADCAST_EAST);
          }
          else {
            XAie_EventBroadcastBlockDir(aieDevInst, loc, XAIE_MEM_MOD, XAIE_EVENT_SWITCH_A, broadcastId1, XAIE_EVENT_BROADCAST_SOUTH | XAIE_EVENT_BROADCAST_WEST | XAIE_EVENT_BROADCAST_EAST | XAIE_EVENT_BROADCAST_NORTH);
          }
        }
        else { //core tile
          if(row != numRows-1) {
            XAie_EventBroadcastBlockDir(aieDevInst, loc, XAIE_CORE_MOD, XAIE_EVENT_SWITCH_A, broadcastId1, XAIE_EVENT_BROADCAST_SOUTH | XAIE_EVENT_BROADCAST_WEST);
          }
          else {
            XAie_EventBroadcastBlockDir(aieDevInst, loc, XAIE_CORE_MOD, XAIE_EVENT_SWITCH_A, broadcastId1, XAIE_EVENT_BROADCAST_SOUTH | XAIE_EVENT_BROADCAST_WEST | XAIE_EVENT_BROADCAST_NORTH);
          }
          XAie_EventBroadcastBlockDir(aieDevInst, loc, XAIE_MEM_MOD, XAIE_EVENT_SWITCH_A, broadcastId1, XAIE_EVENT_BROADCAST_SOUTH | XAIE_EVENT_BROADCAST_WEST | XAIE_EVENT_BROADCAST_EAST | XAIE_EVENT_BROADCAST_NORTH);
        }
      }
    }
  }

  /****************************************************************************
   * Reset 2-channel broadcast network for timer synchronization
   ***************************************************************************/
  void reset2ChannelBroadcastNetwork(XAie_DevInst* aieDevInst, uint8_t broadcastId1,
                                     uint8_t broadcastId2, uint8_t startCol,
                                     uint8_t numCols, uint8_t numRows, uint8_t rowOffset)
  {
    XAie_EventBroadcastReset(aieDevInst, XAie_TileLoc(startCol, 0), XAIE_PL_MOD, broadcastId2);

    for(uint8_t col = startCol; col < startCol + numCols; col++) {
      for(uint8_t row = 0; row < numRows; row++) {
        module_type tileType = getModuleType(row, rowOffset);
        auto loc = XAie_TileLoc(col, row);

        if(tileType == module_type::shim) {
          XAie_EventBroadcastReset(aieDevInst, loc, XAIE_PL_MOD, broadcastId1);
          XAie_EventBroadcastUnblockDir(aieDevInst, loc, XAIE_PL_MOD, XAIE_EVENT_SWITCH_A, broadcastId1, XAIE_EVENT_BROADCAST_ALL);
          XAie_EventBroadcastUnblockDir(aieDevInst, loc, XAIE_PL_MOD, XAIE_EVENT_SWITCH_A, broadcastId2, XAIE_EVENT_BROADCAST_ALL);
          XAie_EventBroadcastUnblockDir(aieDevInst, loc, XAIE_PL_MOD, XAIE_EVENT_SWITCH_B, broadcastId2, XAIE_EVENT_BROADCAST_ALL);
        }
        else if(tileType == module_type::mem_tile) {
          XAie_EventBroadcastUnblockDir(aieDevInst, loc, XAIE_MEM_MOD, XAIE_EVENT_SWITCH_A, broadcastId1, XAIE_EVENT_BROADCAST_ALL);
        }
        else { //core tile
          XAie_EventBroadcastUnblockDir(aieDevInst, loc, XAIE_CORE_MOD, XAIE_EVENT_SWITCH_A, broadcastId1, XAIE_EVENT_BROADCAST_ALL);
          XAie_EventBroadcastUnblockDir(aieDevInst, loc, XAIE_MEM_MOD, XAIE_EVENT_SWITCH_A, broadcastId1, XAIE_EVENT_BROADCAST_ALL);
        }
      }
    }
  }

  /****************************************************************************
   * Synchronize timers across all tiles in the partition
   ***************************************************************************/
  void timerSynchronization(XAie_DevInst* aieDevInst, xaiefal::XAieDev* aieDevice,
                            uint8_t startCol, uint8_t numCols, uint8_t numRows,
                            uint8_t rowOffset)
  {
    std::shared_ptr<xaiefal::XAieBroadcast> broadcastCh1 = nullptr, broadcastCh2 = nullptr;
    std::vector<XAie_LocType> vL;
    broadcastCh1 = aieDevice->broadcast(vL, XAIE_PL_MOD, XAIE_CORE_MOD);
    broadcastCh1->reserve();
    broadcastCh2 = aieDevice->broadcast(vL, XAIE_PL_MOD, XAIE_CORE_MOD);
    broadcastCh2->reserve();

    uint8_t broadcastId1 = broadcastCh1->getBc();
    uint8_t broadcastId2 = broadcastCh2->getBc();

    //build broadcast network
    build2ChannelBroadcastNetwork(aieDevInst, broadcastId1, broadcastId2,
                                  XAIE_EVENT_COMBO_EVENT_0_PL, startCol, numCols, numRows, rowOffset);

    //set timer control register
    for(uint8_t col = startCol; col < startCol + numCols; col++) {
      for(uint8_t row = 0; row < numRows; row++) {
        auto type = getModuleType(row, rowOffset);
        auto loc = XAie_TileLoc(col, row);

        if(type == module_type::shim) {
          XAie_Events resetEvent = (XAie_Events)(XAIE_EVENT_BROADCAST_A_0_PL + broadcastId2);
          if(col == startCol) {
            resetEvent = XAIE_EVENT_COMBO_EVENT_0_PL;
          }
          XAie_SetTimerResetEvent(aieDevInst, loc, XAIE_PL_MOD, resetEvent, XAIE_RESETDISABLE);
        }
        else if(type == module_type::mem_tile) {
          XAie_Events resetEvent = (XAie_Events)(XAIE_EVENT_BROADCAST_0_MEM_TILE + broadcastId1);
          XAie_SetTimerResetEvent(aieDevInst, loc, XAIE_MEM_MOD, resetEvent, XAIE_RESETDISABLE);
        }
        else {
          XAie_Events resetEvent = (XAie_Events)(XAIE_EVENT_BROADCAST_0_CORE + broadcastId1);
          XAie_SetTimerResetEvent(aieDevInst, loc, XAIE_CORE_MOD, resetEvent, XAIE_RESETDISABLE);
          resetEvent = (XAie_Events)(XAIE_EVENT_BROADCAST_0_MEM + broadcastId1);
          XAie_SetTimerResetEvent(aieDevInst, loc, XAIE_MEM_MOD, resetEvent, XAIE_RESETDISABLE);
        }
      }
    }

    //Generate the event to trigger broadcast network to reset timer
    XAie_EventGenerate(aieDevInst, XAie_TileLoc(startCol, 0), XAIE_PL_MOD, XAIE_EVENT_COMBO_EVENT_0_PL);

    //reset timer control register so that timer are not reset after this point
    for(uint8_t col = startCol; col < startCol + numCols; col++) {
      for(uint8_t row = 0; row < numRows; row++) {
        auto type = getModuleType(row, rowOffset);
        auto loc = XAie_TileLoc(col, row);

        if(type == module_type::shim) {
          XAie_Events resetEvent = XAIE_EVENT_NONE_PL;
          XAie_SetTimerResetEvent(aieDevInst, loc, XAIE_PL_MOD, resetEvent, XAIE_RESETDISABLE);
        }
        else if(type == module_type::mem_tile) {
          XAie_Events resetEvent = XAIE_EVENT_NONE_MEM_TILE;
          XAie_SetTimerResetEvent(aieDevInst, loc, XAIE_MEM_MOD, resetEvent, XAIE_RESETDISABLE);
        }
        else {
          XAie_Events resetEvent = XAIE_EVENT_NONE_CORE;
          XAie_SetTimerResetEvent(aieDevInst, loc, XAIE_CORE_MOD, resetEvent, XAIE_RESETDISABLE);
          resetEvent = XAIE_EVENT_NONE_MEM;
          XAie_SetTimerResetEvent(aieDevInst, loc, XAIE_MEM_MOD, resetEvent, XAIE_RESETDISABLE);
        }
      }
    }

    //reset broadcast network
    reset2ChannelBroadcastNetwork(aieDevInst, broadcastId1, broadcastId2, startCol,
                                  numCols, numRows, rowOffset);

    //release the channels used for timer sync
    broadcastCh1->release();
    broadcastCh2->release();
  }

}  // namespace xdp::aie

