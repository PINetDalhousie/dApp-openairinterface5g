/*
 * Licensed to the OpenAirInterface (OAI) Software Alliance under one or more
 * contributor license agreements.  See the NOTICE file distributed with
 * this work for additional information regarding copyright ownership.
 * The OpenAirInterface Software Alliance licenses this file to You under
 * the OAI Public License, Version 1.1  (the "License"); you may not use this file
 * except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.openairinterface.org/?page_id=698
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *-------------------------------------------------------------------------------
 * For more information about the OpenAirInterface (OAI) Software Alliance:
 *      contact@openairinterface.org
 */

/*! \file mac.h
* \brief MAC data structures, constant, and function prototype
* \author Navid Nikaein and Raymond Knopp, WIE-TAI CHEN
* \date 2011, 2018
* \version 0.5
* \company Eurecom, NTUST
* \email navid.nikaein@eurecom.fr, kroempa@gmail.com

*/
/** @defgroup _oai2  openair2 Reference Implementation
 * @ingroup _ref_implementation_
 * @{
 */

/*@}*/

#ifndef __LAYER2_NR_MAC_GNB_H__
#define __LAYER2_NR_MAC_GNB_H__

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include "common/utils/ds/seq_arr.h"
#include "common/utils/nr/nr_common.h"
#include "common/utils/ds/byte_array.h"

#define NR_SCHED_LOCK(lock)                                        \
  do {                                                             \
    int rc = pthread_mutex_lock(lock);                             \
    AssertFatal(rc == 0, "error while locking scheduler mutex, pthread_mutex_lock() returned %d\n", rc); \
  } while (0)

#define NR_SCHED_UNLOCK(lock)                                      \
  do {                                                             \
    int rc = pthread_mutex_unlock(lock);                           \
    AssertFatal(rc == 0, "error while locking scheduler mutex, pthread_mutex_unlock() returned %d\n", rc); \
  } while (0)

#define NR_SCHED_ENSURE_LOCKED(lock)\
  do {\
    int rc = pthread_mutex_trylock(lock); \
    AssertFatal(rc == EBUSY, "this function should be called with the scheduler mutex locked, pthread_mutex_trylock() returned %d\n", rc);\
  } while (0)

/* Commmon */
#include "radio/COMMON/common_lib.h"
#include "common/platform_constants.h"
#include "common/ran_context.h"
#include "collection/linear_alloc.h"

/* RRC */
#include "NR_BCCH-BCH-Message.h"
#include "NR_CellGroupConfig.h"
#include "NR_BCCH-DL-SCH-Message.h"
#include "openair2/RRC/NR/nr_rrc_config.h"

/* PHY */
#include "time_meas.h"

/* Interface */
#include "nfapi_nr_interface_scf.h"
#include "nfapi_nr_interface.h"
#include "NR_PHY_INTERFACE/NR_IF_Module.h"
#include "mac_rrc_ul.h"

/* MAC */
#include "LAYER2/NR_MAC_COMMON/nr_mac_extern.h"
#include "LAYER2/NR_MAC_COMMON/nr_mac_common.h"
#include "NR_TAG.h"

#include <openair3/UICC/usim_interface.h>


/* Defs */
#define MAX_NUM_BWP 5
#define MAX_NUM_CORESET 12
#define MAX_NUM_CCE 90
/*!\brief Maximum number of random access process */
#define NR_NB_RA_PROC_MAX 4
#define MAX_NUM_OF_SSB 64
#define MAX_NUM_NR_PRACH_PREAMBLES 64
#define MIN_NUM_PRBS_TO_SCHEDULE  5

extern const uint8_t nr_rv_round_map[4];

/*! \brief NR_list_t is a "list" (of users, HARQ processes, slices, ...).
 * Especially useful in the scheduler and to keep "classes" of users. */
typedef struct {
  int head;
  int *next;
  int tail;
  int len;
} NR_list_t;

typedef enum {
  nrRA_gNB_IDLE,
  nrRA_Msg2,
  nrRA_WAIT_MsgA_PUSCH,
  nrRA_WAIT_Msg3,
  nrRA_Msg3_retransmission,
  nrRA_Msg4,
  nrRA_MsgB,
  nrRA_WAIT_Msg4_MsgB_ACK,
} RA_gNB_state_t;

static const char *const nrra_text[] =
    {"IDLE", "Msg2", "WAIT_MsgA_PUSCH", "WAIT_Msg3", "Msg3_retransmission", "Msg4", "MsgB", "WAIT_Msg4_MsgB_ACK"};

typedef struct {
  int idx;
  bool new_beam;
} NR_beam_alloc_t;

typedef struct nr_pdsch_AntennaPorts_t {
  int N1;
  int N2;
  int XP;
} nr_pdsch_AntennaPorts_t;

typedef struct nr_mac_timers {
  int sr_ProhibitTimer;
  int sr_TransMax;
  int sr_ProhibitTimer_v1700;
  int t300;
  int t301;
  int t310;
  int n310;
  int t311;
  int n311;
  int t319;
} nr_mac_timers_t;

typedef struct nr_mac_config_t {
  int sib1_tda;
  nr_pdsch_AntennaPorts_t pdsch_AntennaPorts;
  int pusch_AntennaPorts;
  int minRXTXTIME;
  int do_CSIRS;
  int do_SRS;
  bool force_256qam_off;
  bool force_UL256qam_off;
  bool use_deltaMCS;
  int maxMIMO_layers;
  bool disable_harq;
  //int pusch_TargetSNRx10;
  //int pucch_TargetSNRx10;
  nr_mac_timers_t timer_config;
  int num_dlharq;
  int num_ulharq;
  /// beamforming weight matrix size
  int nb_bfw[2];
  int32_t *bw_list;
  int num_agg_level_candidates[NUM_PDCCH_AGG_LEVELS];
} nr_mac_config_t;

typedef struct NR_preamble_ue {
  uint8_t num_preambles;
  uint8_t preamble_list[MAX_NUM_NR_PRACH_PREAMBLES];
} NR_preamble_ue_t;

typedef struct NR_sched_pdcch {
  uint16_t BWPSize;
  uint16_t BWPStart;
  uint8_t CyclicPrefix;
  uint8_t SubcarrierSpacing;
  uint8_t StartSymbolIndex;
  uint8_t CceRegMappingType;
  uint8_t RegBundleSize;
  uint8_t InterleaverSize;
  uint16_t ShiftIndex;
  uint8_t DurationSymbols;
  int n_rb;
} NR_sched_pdcch_t;

/*! \brief gNB template for the Random access information */
typedef struct {
  /// Flag to indicate this process is active
  RA_gNB_state_t ra_state;
  /// CORESET0 configured flag
  int coreset0_configured;
  /// Frame where preamble was received
  int preamble_frame;
  /// Slot where preamble was received
  uint8_t preamble_slot;
  /// Received preamble_index
  uint8_t preamble_index;
  /// Timing offset indicated by PHY
  int16_t timing_offset;
  /// Subframe where Msg2 is to be sent
  uint8_t Msg2_slot;
  /// Frame where Msg2 is to be sent
  frame_t Msg2_frame;
  /// Subframe where Msg3 is to be sent
  sub_frame_t Msg3_slot;
  /// Frame where Msg3 is to be sent
  frame_t Msg3_frame;
  /// Msg3 time domain allocation index
  int Msg3_tda_id;
  /// Msg3 beam matrix index
  NR_beam_alloc_t Msg3_beam;
  /// harq_pid used for Msg4 transmission
  uint8_t harq_pid;
  /// UE RNTI allocated during RAR
  rnti_t rnti;
  /// RA RNTI allocated from received PRACH
  uint16_t RA_rnti;
  /// MsgB RNTI allocated from received MsgA
  uint16_t MsgB_rnti;
  /// Received UE Contention Resolution Identifier
  uint8_t cont_res_id[6];
  /// Msg3 first RB
  int msg3_first_rb;
  /// Msg3 number of RB
  int msg3_nb_rb;
  /// Msg3 BWP start
  int msg3_bwp_start;
  /// Msg3 TPC command
  uint8_t msg3_TPC;
  /// Round of Msg3 HARQ
  uint8_t msg3_round;
  int msg3_startsymb;
  int msg3_nbSymb;
  /// MAC PDU length for Msg4
  int mac_pdu_length;
  /// RA search space
  NR_SearchSpace_t *ra_ss;
  /// RA Coreset
  NR_ControlResourceSet_t *coreset;
  NR_sched_pdcch_t sched_pdcch;
  // Beam index
  uint8_t beam_id;
  /// CellGroup for UE that is to come (NSA is non-null, null for SA)
  NR_CellGroupConfig_t *CellGroup;
  /// Preambles for contention-free access
  NR_preamble_ue_t preambles;
  int contention_resolution_timer;
  nr_ra_type_t ra_type;
  /// CFRA flag
  bool cfra;
  // BWP for RA
  NR_UE_DL_BWP_t DL_BWP;
  NR_UE_UL_BWP_t UL_BWP;
  NR_UE_ServingCell_Info_t sc_info;
} NR_RA_t;

/*! \brief gNB common channels */
typedef struct {
  frame_type_t frame_type;
  NR_BCCH_BCH_Message_t *mib;
  NR_BCCH_DL_SCH_Message_t *sib1;
  NR_BCCH_DL_SCH_Message_t *sib19;
  NR_ServingCellConfigCommon_t *ServingCellConfigCommon;
  /// pre-configured ServingCellConfig that is default for every UE
  NR_ServingCellConfig_t *pre_ServingCellConfig;
  /// Outgoing MIB PDU for PHY
  uint8_t MIB_pdu[3];
  /// Outgoing BCCH pdu for PHY
  uint8_t sib1_bcch_pdu[NR_MAX_SIB_LENGTH / 8];
  int sib1_bcch_length;
  /// used for sib19 data
  uint8_t sib19_bcch_pdu[NR_MAX_SIB_LENGTH / 8];
  int sib19_bcch_length;
  /// Template for RA computations
  NR_RA_t ra[NR_NB_RA_PROC_MAX];
  /// VRB map for common channels
  uint16_t vrb_map[MAX_NUM_BEAM_PERIODS][275];
  /// VRB map for common channels and PUSCH, dynamically allocated because
  /// length depends on number of slots and RBs
  uint16_t *vrb_map_UL[MAX_NUM_BEAM_PERIODS];
  ///Number of active SSBs
  int num_active_ssb;
  //Total available prach occasions per configuration period
  int total_prach_occasions_per_config_period;
  //Total available prach occasions
  int total_prach_occasions;
  //Max Association period
  int max_association_period;
  //SSB index
  uint8_t ssb_index[MAX_NUM_OF_SSB];
  //CB preambles for each SSB
  int cb_preambles_per_ssb;
} NR_COMMON_channels_t;

// SP ZP CSI-RS Resource Set Activation/Deactivation MAC CE
typedef struct sp_zp_csirs {
  bool is_scheduled;     //ZP CSI-RS ACT/Deact MAC CE is scheduled
  bool act_deact;        //Activation/Deactivation indication
  uint8_t serv_cell_id;  //Identity of Serving cell for which MAC CE applies
  uint8_t bwpid;         //Downlink BWP id
  uint8_t rsc_id;        //SP ZP CSI-RS resource set
} sp_zp_csirs_t;

//SP CSI-RS / CSI-IM Resource Set Activation/Deactivation MAC CE
#define MAX_CSI_RESOURCE_SET 64
typedef struct csi_rs_im {
  bool is_scheduled;
  bool act_deact;
  uint8_t serv_cellid;
  uint8_t bwp_id;
  bool im;
  uint8_t csi_im_rsc_id;
  uint8_t nzp_csi_rsc_id;
  uint8_t nb_tci_resource_set_id;
  uint8_t tci_state_id [ MAX_CSI_RESOURCE_SET ];
} csi_rs_im_t;

typedef struct pdcchStateInd {
  bool is_scheduled;
  uint8_t servingCellId;
  uint8_t coresetId;
  uint8_t tciStateId;
  bool tci_present_inDCI;
} pdcchStateInd_t;

typedef struct pucchSpatialRelation {
  bool is_scheduled;
  uint8_t servingCellId;
  uint8_t bwpId;
  uint8_t pucchResourceId;
  bool s0tos7_actDeact[8];
} pucchSpatialRelation_t;

typedef struct SPCSIReportingpucch {
  bool is_scheduled;
  uint8_t servingCellId;
  uint8_t bwpId;
  bool s0tos3_actDeact[4];
} SPCSIReportingpucch_t;

#define MAX_APERIODIC_TRIGGER_STATES 128 //38.331                               
typedef struct aperiodicCSI_triggerStateSelection {
  bool is_scheduled;
  uint8_t servingCellId;
  uint8_t bwpId;
  uint8_t highestTriggerStateSelected;
  bool triggerStateSelection[MAX_APERIODIC_TRIGGER_STATES];
} aperiodicCSI_triggerStateSelection_t;

#define MAX_TCI_STATES 128 //38.331                                             
typedef struct pdschTciStatesActDeact {
  bool is_scheduled;
  uint8_t servingCellId;
  uint8_t bwpId;
  uint8_t highestTciStateActivated;
  bool tciStateActDeact[MAX_TCI_STATES];
  uint8_t codepoint[8];
} pdschTciStatesActDeact_t;

typedef struct UE_info {
  sp_zp_csirs_t sp_zp_csi_rs;
  csi_rs_im_t csi_im;
  pdcchStateInd_t pdcch_state_ind;
  pucchSpatialRelation_t pucch_spatial_relation;
  SPCSIReportingpucch_t SP_CSI_reporting_pucch;
  aperiodicCSI_triggerStateSelection_t aperi_CSI_trigger;
  pdschTciStatesActDeact_t pdsch_TCI_States_ActDeact;
} NR_UE_mac_ce_ctrl_t;

typedef struct NR_sched_pucch {
  bool active;
  int frame;
  int ul_slot;
  bool sr_flag;
  int csi_bits;
  bool simultaneous_harqcsi;
  uint8_t dai_c;
  uint8_t timing_indicator;
  uint8_t resource_indicator;
  int r_pucch;
  int prb_start;
  int second_hop_prb;
  int nr_of_symb;
  int start_symb;
} NR_sched_pucch_t;

typedef struct NR_pusch_dmrs {
  uint8_t N_PRB_DMRS;
  uint8_t num_dmrs_symb;
  uint16_t ul_dmrs_symb_pos;
  uint8_t num_dmrs_cdm_grps_no_data;
  nfapi_nr_dmrs_type_e dmrs_config_type;
} NR_pusch_dmrs_t;

typedef struct NR_sched_pusch {
  int frame;
  int slot;
  int mu;

  /// RB allocation within active uBWP
  uint16_t rbSize;
  uint16_t rbStart;

  /// MCS
  uint8_t mcs;

  /// TBS-related info
  uint16_t R;
  uint8_t Qm;
  uint32_t tb_size;

  /// UL HARQ PID to use for this UE, or -1 for "any new"
  int8_t ul_harq_pid;

  uint8_t nrOfLayers;
  int tpmi;

  // time_domain_allocation is the index of a list of tda
  int time_domain_allocation;
  NR_tda_info_t tda_info;
  NR_pusch_dmrs_t dmrs_info;
  int phr_txpower_calc;
} NR_sched_pusch_t;

typedef struct NR_sched_srs {
  int frame;
  int slot;
  bool srs_scheduled;
} NR_sched_srs_t;

typedef struct NR_pdsch_dmrs {
  uint8_t dmrs_ports_id;
  uint8_t N_PRB_DMRS;
  uint8_t N_DMRS_SLOT;
  uint16_t dl_dmrs_symb_pos;
  uint8_t numDmrsCdmGrpsNoData;
  nfapi_nr_dmrs_type_e dmrsConfigType;
} NR_pdsch_dmrs_t;

typedef struct NR_sched_pdsch {
  /// RB allocation within active BWP
  uint16_t rbSize;
  uint16_t rbStart;

  /// MCS-related infos
  uint8_t mcs;

  /// TBS-related info
  uint16_t R;
  uint8_t Qm;
  uint32_t tb_size;

  /// DL HARQ PID to use for this UE, or -1 for "any new"
  int8_t dl_harq_pid;

  // pucch format allocation
  int16_t pucch_allocation;

  uint16_t pm_index;
  uint8_t nrOfLayers;

  NR_pdsch_dmrs_t dmrs_parms;
  // time_domain_allocation is the index of a list of tda
  int time_domain_allocation;
  NR_tda_info_t tda_info;
} NR_sched_pdsch_t;

typedef struct NR_UE_harq {
  bool is_waiting;
  uint8_t ndi;
  uint8_t round;
  uint16_t feedback_frame;
  uint16_t feedback_slot;

  /* Transport block to be sent using this HARQ process */
  byte_array_t transportBlock;
  uint32_t tb_size;  // size of currently stored TB

  /// sched_pdsch keeps information on MCS etc used for the initial transmission
  NR_sched_pdsch_t sched_pdsch;
} NR_UE_harq_t;

//! fixme : need to enhace for the multiple TB CQI report

typedef struct NR_bler_stats {
  frame_t last_frame;
  float bler;
  uint8_t mcs;
  uint64_t rounds[8];
} NR_bler_stats_t;

//
/*! As per spec 38.214 section 5.2.1.4.2
 * - if the UE is configured with the higher layer parameter groupBasedBeamReporting set to 'disabled', the UE shall report in
  a single report nrofReportedRS (higher layer configured) different CRI or SSBRI for each report setting.
 * - if the UE is configured with the higher layer parameter groupBasedBeamReporting set to 'enabled', the UE shall report in a
  single reporting instance two different CRI or SSBRI for each report setting, where CSI-RS and/or SSB
  resources can be received simultaneously by the UE either with a single spatial domain receive filter, or with
  multiple simultaneous spatial domain receive filter
*/
#define MAX_NR_OF_REPORTED_RS 4

struct CRI_RI_LI_PMI_CQI {
  uint8_t cri;
  uint8_t ri;
  uint8_t li;
  uint8_t pmi_x1;
  uint8_t pmi_x2;
  uint8_t wb_cqi_1tb;
  uint8_t wb_cqi_2tb;
  uint8_t cqi_table;
  uint8_t csi_report_id;
  bool print_report;
};

typedef struct RSRP_report {
  uint8_t nr_reports;
  uint8_t resource_id[MAX_NR_OF_REPORTED_RS];
  int RSRP[MAX_NR_OF_REPORTED_RS];
} RSRP_report_t;

struct CSI_Report {
  struct CRI_RI_LI_PMI_CQI cri_ri_li_pmi_cqi_report;
  RSRP_report_t ssb_rsrp_report;
  RSRP_report_t csirs_rsrp_report;
};

#define MAX_SR_BITLEN 8

/*! As per the spec 38.212 and table:  6.3.1.1.2-12 in a single UCI sequence we can have multiple CSI_report 
  the number of CSI_report will depend on number of CSI resource sets that are configured in CSI-ResourceConfig RRC IE
  From spec 38.331 from the IE CSI-ResourceConfig for SSB RSRP reporting we can configure only one resource set 
  From spec 38.214 section 5.2.1.2 For periodic and semi-persistent CSI Resource Settings, the number of CSI-RS Resource Sets configured is limited to S=1
 */
#define MAX_CSI_RESOURCE_SET_IN_CSI_RESOURCE_CONFIG 16

typedef enum {
  INACTIVE = 0,
  ACTIVE_NOT_SCHED,
  ACTIVE_SCHED
} NR_UL_harq_states_t;

typedef struct NR_UE_ul_harq {
  bool is_waiting;
  uint8_t ndi;
  uint8_t round;
  uint16_t feedback_slot;

  /// sched_pusch keeps information on MCS etc used for the initial transmission
  NR_sched_pusch_t sched_pusch;
} NR_UE_ul_harq_t;

typedef struct NR_QoS_config_s {
  int fiveQI;
  int priority;
} NR_QoS_config_t;

typedef struct nr_lc_config {
  uint8_t lcid;
  /// priority as specified in 38.321
  int priority;
  /// associated NSSAI for DRB
  nssai_t nssai;
  /// QoS config for DRB
  NR_QoS_config_t qos_config[NR_MAX_NUM_QFI];
} nr_lc_config_t;

/*! \brief scheduling control information set through an API */
#define MAX_CSI_REPORTS 48
typedef struct {
  /// CCE index and aggregation, should be coherent with cce_list
  NR_SearchSpace_t *search_space;
  NR_ControlResourceSet_t *coreset;
  NR_sched_pdcch_t sched_pdcch;

  /// CCE index and Aggr. Level are shared for PUSCH/PDSCH allocation decisions
  /// corresponding to the sched_pusch/sched_pdsch structures below
  int cce_index;
  uint8_t aggregation_level;

  /// Array of PUCCH scheduling information
  /// Its size depends on TDD configuration and max feedback time
  /// There will be a structure for each UL slot in the active period determined by the size
  NR_sched_pucch_t *sched_pucch;
  int sched_pucch_size;

  /// Sched PUSCH: scheduling decisions, copied into HARQ and cleared every TTI
  NR_sched_pusch_t sched_pusch;

  /// Sched SRS: scheduling decisions
  NR_sched_srs_t sched_srs;

  /// uplink bytes that are currently scheduled
  int sched_ul_bytes;
  /// estimation of the UL buffer size
  int estimated_ul_buffer;

  /// PHR info: power headroom level (dB)
  int ph;
  /// PHR info: power headroom level (dB) for 1 PRB
  int ph0;

  /// PHR info: nominal UE transmit power levels (dBm)
  int pcmax;

  /// Sched PDSCH: scheduling decisions, copied into HARQ and cleared every TTI
  NR_sched_pdsch_t sched_pdsch;
  /// UE-estimated maximum MCS (from CSI-RS)
  uint8_t dl_max_mcs;

  /// For UL synchronization: store last UL scheduling grant
  frame_t last_ul_frame;
  sub_frame_t last_ul_slot;

  /// total amount of data awaiting for this UE
  uint32_t num_total_bytes;
  uint16_t dl_pdus_total;
  /// per-LC status data
  mac_rlc_status_resp_t rlc_status[NR_MAX_NUM_LCID];

  /// Estimation of HARQ from BLER
  NR_bler_stats_t dl_bler_stats;
  NR_bler_stats_t ul_bler_stats;

  uint16_t ta_frame;
  int16_t ta_update;
  bool ta_apply;
  uint8_t tpc0;
  uint8_t tpc1;
  int raw_rssi;
  int pusch_snrx10;
  int pucch_snrx10;
  uint16_t ul_rssi;
  uint8_t current_harq_pid;
  int pusch_consecutive_dtx_cnt;
  int pucch_consecutive_dtx_cnt;
  bool ul_failure;
  int ul_failure_timer;
  int release_timer;
  struct CSI_Report CSI_report;
  bool SR;
  /// information about every HARQ process
  NR_UE_harq_t harq_processes[NR_MAX_HARQ_PROCESSES];
  /// HARQ processes that are free
  NR_list_t available_dl_harq;
  /// HARQ processes that await feedback
  NR_list_t feedback_dl_harq;
  /// HARQ processes that await retransmission
  NR_list_t retrans_dl_harq;
  /// information about every UL HARQ process
  NR_UE_ul_harq_t ul_harq_processes[NR_MAX_HARQ_PROCESSES];
  /// UL HARQ processes that are free
  NR_list_t available_ul_harq;
  /// UL HARQ processes that await feedback
  NR_list_t feedback_ul_harq;
  /// UL HARQ processes that await retransmission
  NR_list_t retrans_ul_harq;
  NR_UE_mac_ce_ctrl_t UE_mac_ce_ctrl; // MAC CE related information

  /// Timer for RRC processing procedures and transmission activity
  NR_timer_t transm_interrupt;

  /// sri, ul_ri and tpmi based on SRS
  nr_srs_feedback_t srs_feedback;

  /// per-LC configuration
  seq_arr_t lc_config;

  // pdcch closed loop adjust for PDCCH aggregation level, range <0, 1>
  // 0 - good channel, 1 - bad channel
  float pdcch_cl_adjust;
} NR_UE_sched_ctrl_t;

typedef struct {
  NR_SearchSpace_t *search_space;
  NR_ControlResourceSet_t *coreset;

  NR_sched_pdcch_t sched_pdcch;
  NR_sched_pdsch_t sched_pdsch;

  uint32_t num_total_bytes;

  int cce_index;
  uint8_t aggregation_level;
} NR_UE_sched_osi_ctrl_t;

typedef struct {
  uicc_t *uicc;
} NRUEcontext_t;

typedef struct NR_mac_dir_stats {
  uint64_t lc_bytes[64];
  uint64_t rounds[8];
  uint64_t errors;
  uint64_t total_bytes;
  uint32_t current_bytes;
  uint64_t total_sdu_bytes;
  uint32_t total_rbs;
  uint32_t total_rbs_retx;
  uint32_t num_mac_sdu;
  uint32_t current_rbs;
} NR_mac_dir_stats_t;

typedef struct NR_mac_stats {
  NR_mac_dir_stats_t dl;
  NR_mac_dir_stats_t ul;
  uint32_t ulsch_DTX;
  uint64_t ulsch_total_bytes_scheduled;
  uint32_t pucch0_DTX;
  int cumul_rsrp;
  uint8_t num_rsrp_meas;
  char srs_stats[50]; // Statistics may differ depending on SRS usage
  int pusch_snrx10;
  int deltaMCS;
  int NPRB;
} NR_mac_stats_t;

typedef struct NR_bler_options {
  double upper;
  double lower;
  uint8_t max_mcs;
  uint8_t harq_round_max;
} NR_bler_options_t;

typedef struct nr_mac_rrc_ul_if_s {
  f1_reset_du_initiated_func_t f1_reset;
  f1_reset_acknowledge_cu_initiated_func_t f1_reset_acknowledge;
  f1_setup_request_func_t f1_setup_request;
  gnb_du_configuration_update_t gnb_du_configuration_update;
  ue_context_setup_response_func_t ue_context_setup_response;
  ue_context_modification_response_func_t ue_context_modification_response;
  ue_context_modification_required_func_t ue_context_modification_required;
  ue_context_release_request_func_t ue_context_release_request;
  ue_context_release_complete_func_t ue_context_release_complete;
  initial_ul_rrc_message_transfer_func_t initial_ul_rrc_message_transfer;
} nr_mac_rrc_ul_if_t;

typedef enum interrupt_followup_action { FOLLOW_INSYNC, FOLLOW_INSYNC_RECONFIG, FOLLOW_OUTOFSYNC  } interrupt_followup_action_t;

/*! \brief UE list used by gNB to order UEs/CC for scheduling*/
typedef struct {
  rnti_t rnti;
  uid_t uid; // unique ID of this UE
  /// scheduling control info
  nr_csi_report_t csi_report_template[MAX_CSI_REPORTCONFIG];
  NR_UE_sched_ctrl_t UE_sched_ctrl;
  NR_UE_DL_BWP_t current_DL_BWP;
  NR_UE_UL_BWP_t current_UL_BWP;
  NR_UE_ServingCell_Info_t sc_info;
  NR_mac_stats_t mac_stats;
  /// currently active CellGroupConfig
  NR_CellGroupConfig_t *CellGroup;
  /// CellGroupConfig that is to be activated after the next reconfiguration
  bool expect_reconfiguration;
  /// reestablishRLC has to be signaled in RRCreconfiguration
  bool reestablish_rlc;
  NR_CellGroupConfig_t *reconfigCellGroup;
  interrupt_followup_action_t interrupt_action;
  NR_UE_NR_Capability_t *capability;
  // UE selected beam index
  uint8_t UE_beam_index;
  bool Msg4_MsgB_ACKed;
  float ul_thr_ue;
  float dl_thr_ue;
  long pdsch_HARQ_ACK_Codebook;
} NR_UE_info_t;

typedef struct {
  /// scheduling control info
  // last element always NULL
  pthread_mutex_t mutex;
  NR_UE_info_t *list[MAX_MOBILES_PER_GNB+1];
  // bitmap of CSI-RS already scheduled in current slot
  int sched_csirs;
  uid_allocator_t uid_allocator;
} NR_UEs_t;

typedef struct {
  /// list of allocated beams per period
  int **beam_allocation;
  int beam_duration; // in slots
  int beams_per_period;
  int beam_allocation_size;
} NR_beam_info_t;

#define UE_iterator(BaSe, VaR) NR_UE_info_t ** VaR##pptr=BaSe, *VaR; while ((VaR=*(VaR##pptr++)))

typedef void (*nr_pp_impl_dl)(module_id_t mod_id,
                              frame_t frame,
                              sub_frame_t slot);
typedef bool (*nr_pp_impl_ul)(module_id_t mod_id,
                              frame_t frame,
                              sub_frame_t slot);

typedef struct f1_config_t {
  f1ap_setup_req_t *setup_req;
  f1ap_setup_resp_t *setup_resp;
  uint32_t gnb_id; // associated gNB's ID, not used in DU itself
} f1_config_t;

typedef struct {
  char *nvipc_shm_prefix;
  int8_t nvipc_poll_core;
} nvipc_params_t;

typedef struct {
  uint64_t total_prb_aggregate;
  uint64_t used_prb_aggregate;
} mac_stats_t;

/*! \brief top level eNB MAC structure */
typedef struct gNB_MAC_INST_s {
  /// Ethernet parameters for northbound midhaul interface
  eth_params_t                    eth_params_n;
  /// address for F1U to bind, ports in eth_params_n
  char *f1u_addr;
  /// Ethernet parameters for fronthaul interface
  eth_params_t                    eth_params_s;
  /// Nvipc parameters for FAPI interface with Aerial
  nvipc_params_t nvipc_params_s;
  /// Module
  module_id_t                     Mod_id;
  /// timing advance group
  NR_TAG_t                        *tag;
  /// Pointer to IF module instance for PHY
  NR_IF_Module_t                  *if_inst;
  pthread_t                       stats_thread;
#ifdef E3_AGENT
  pthread_t                       prb_update_thread;
#endif // E3_AGENT
  /// Pusch target SNR
  int                             pusch_target_snrx10;
  /// RSSI threshold for power control. Limits power control commands when RSSI reaches threshold.
  int                             pusch_rssi_threshold;
  /// Pucch target SNR
  int                             pucch_target_snrx10;
  /// RSSI threshold for PUCCH power control. Limits power control commands when RSSI reaches threshold.
  int                             pucch_rssi_threshold;
  /// SNR threshold needed to put or not a PRB in the black list
  int                             ul_prbblack_SNR_threshold;
  /// PUCCH Failure threshold (compared to consecutive PUCCH DTX)
  int                             pucch_failure_thres;
  /// PUSCH Failure threshold (compared to consecutive PUSCH DTX)
  int                             pusch_failure_thres;
  /// Subcarrier Offset
  int                             ssb_SubcarrierOffset;
  int                             ssb_OffsetPointA;

  /// Common cell resources
  NR_COMMON_channels_t common_channels[NFAPI_CC_MAX];
  /// current PDU index (BCH,DLSCH)
  uint16_t pdu_index[NFAPI_CC_MAX];
  /// UL PRBs blacklist
  uint16_t ulprbbl[MAX_BWP_SIZE];
#ifdef E3_AGENT
  uint16_t dyn_prbbl[MAX_BWP_SIZE];
#endif // E3_AGENT
  /// NFAPI Config Request Structure
  nfapi_nr_config_request_scf_t     config[NFAPI_CC_MAX];
  /// a PDCCH PDU groups DCIs per BWP and CORESET. The following structure
  /// keeps pointers to PDCCH PDUs within DL_req so that we can easily track
  /// PDCCH PDUs per CC/BWP/CORESET
  nfapi_nr_dl_tti_pdcch_pdu_rel15_t *pdcch_pdu_idx[NFAPI_CC_MAX][MAX_NUM_CORESET];
  /// NFAPI UL TTI Request Structure for future TTIs, dynamically allocated
  /// because length depends on number of slots
  nfapi_nr_ul_tti_request_t        *UL_tti_req_ahead[NFAPI_CC_MAX];
  int UL_tti_req_ahead_size;
  int vrb_map_UL_size;

  NR_UEs_t UE_info;

  /// UL handle
  uint32_t ul_handle;
  //UE_info_t UE_info;

  // MAC function execution peformance profiler
  /// processing time of eNB scheduler
  time_stats_t eNB_scheduler;
  /// processing time of eNB scheduler for SI
  time_stats_t schedule_si;
  /// processing time of eNB scheduler for Random access
  time_stats_t schedule_ra;
  /// processing time of eNB ULSCH scheduler
  time_stats_t schedule_ulsch;
  /// processing time of eNB DCI generation
  time_stats_t fill_DLSCH_dci;
  /// processing time of eNB MAC preprocessor
  time_stats_t schedule_dlsch_preprocessor;
  /// processing time of eNB DLSCH scheduler
  time_stats_t schedule_dlsch;  // include rlc_data_req + MAC header + preprocessor
  /// processing time of rlc_data_req
  time_stats_t rlc_data_req;
  /// processing time of rlc_status_ind
  time_stats_t rlc_status_ind;
  /// processing time of nr_srs_ri_computation
  time_stats_t nr_srs_ri_computation_timer;
  /// processing time of nr_srs_tpmi_estimation
  time_stats_t nr_srs_tpmi_computation_timer;
  /// processing time of eNB MCH scheduler
  time_stats_t schedule_mch;
  /// processing time of eNB ULSCH reception
  time_stats_t rx_ulsch_sdu;  // include rlc_data_ind
  /// processing time of eNB PCH scheduler
  time_stats_t schedule_pch;

  NR_beam_info_t beam_info;

  /// maximum number of slots before a UE will be scheduled ULSCH automatically
  uint32_t ulsch_max_frame_inactivity;
  /// instance of the frame structure configuration
  frame_structure_t frame_structure;

  /// DL preprocessor for differentiated scheduling
  nr_pp_impl_dl pre_processor_dl;
  /// UL preprocessor for differentiated scheduling
  nr_pp_impl_ul pre_processor_ul;

  nr_mac_config_t radio_config;

  NR_UE_sched_osi_ctrl_t *sched_osi;
  NR_UE_sched_ctrl_t *sched_ctrlCommon;

  uint16_t cset0_bwp_start;
  uint16_t cset0_bwp_size;
  NR_Type0_PDCCH_CSS_config_t type0_PDCCH_CSS_config[64];

  bool first_MIB;
  NR_bler_options_t dl_bler;
  NR_bler_options_t ul_bler;
  uint8_t min_grant_prb;
  uint8_t min_grant_mcs;
  bool identity_pm;
  int precoding_matrix_size[NR_MAX_NB_LAYERS];
  int fapi_beam_index[MAX_NUM_OF_SSB];
  nr_mac_rrc_ul_if_t mac_rrc;
  f1_config_t f1_config;
  int16_t frame;

  pthread_mutex_t sched_lock;

  mac_stats_t mac_stats;

} gNB_MAC_INST;

#endif /*__LAYER2_NR_MAC_GNB_H__ */
/** @}*/
