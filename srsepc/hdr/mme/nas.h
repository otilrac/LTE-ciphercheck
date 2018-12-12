/*
 * \section LICENSE
 *
 * This file is part of srsLTE.
 *
 * srsLTE is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License as
 * published by the Free Software Foundation, either version 3 of
 * the License, or (at your option) any later version.
 *
 * srsLTE is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Affero General Public License for more details.
 *
 * A copy of the GNU Affero General Public License can be found in
 * the LICENSE file in the top-level directory of this distribution
 * and at http://www.gnu.org/licenses/.
 *
 */
#ifndef SRSEPC_NAS_H
#define SRSEPC_NAS_H

#include <netinet/sctp.h>

#include "srslte/common/security.h"
#include "srslte/asn1/gtpc_ies.h"
#include "srslte/asn1/liblte_s1ap.h"
#include "srslte/asn1/liblte_mme.h"
#include "srslte/common/buffer_pool.h"
#include "srslte/interfaces/epc_interfaces.h"

namespace srsepc{

static const uint8_t MAX_ERABS_PER_UE = 16;

// MME EMM states (3GPP 24.301 v10.0.0, section 5.1.3.4)
typedef enum {
  EMM_STATE_DEREGISTERED = 0,
  EMM_STATE_COMMON_PROCEDURE_INITIATED,
  EMM_STATE_REGISTERED,
  EMM_STATE_DEREGISTERED_INITIATED,
  EMM_STATE_N_ITEMS,
} emm_state_t;
static const char emm_state_text[EMM_STATE_N_ITEMS][100] = {"DEREGISTERED",
                                                            "COMMON PROCEDURE INITIATED",
                                                            "REGISTERED",
                                                            "DEREGISTERED INITIATED"};

// MME ECM states (3GPP 23.401 v10.0.0, section 4.6.3)
typedef enum {
  ECM_STATE_IDLE = 0,
  ECM_STATE_CONNECTED,
  ECM_STATE_N_ITEMS,
} ecm_state_t;
static const char ecm_state_text[ECM_STATE_N_ITEMS][100] = {"IDLE",
                                                            "CONNECTED"};
/*
// MME ESM states (3GPP 23.401 v10.0.0, section 4.6.3)
typedef enum {
  ESM_BEARER_CONTEXT_INACTIVE = 0,
  ESM_BEARER_CONTEXT_ACTIVE_PENDING,
  ESM_BEARER_CONTEXT_ACTIVE,
  ESM_BEARER_CONTEXT_INACTIVE_PENDING,
  ESM_BEARER_CONTEXT_MODIFY_PENDING,
  ESM_BEARER_PROCEDURE_TRANSACTION_INACTIVE,
  ESM_BEARER_PROCEDURE_TRANSACTION_PENDING,
  ESM_STATE_N_ITEMS,
} esm_state_t;
static const char esm_state_text[ESM_STATE_N_ITEMS][100] = {"CONTEXT INACTIVE",
                                                            "CONTEXT ACTIVE PENDING",
                                                            "CONTEXT ACTIVE",
                                                            "CONTEXT_INACTIVE_PENDING",
                                                            "CONTEXT_MODIFY_PENDING",
                                                            "PROCEDURE_TRANSACTION_INACTIVE"
                                                            "PROCEDURE_TRANSACTION_PENDING"};
*/
typedef enum
{
  ERAB_DEACTIVATED,
  ERAB_CTX_REQUESTED,
  ERAB_CTX_SETUP,
  ERAB_ACTIVE
} esm_state_t;

/*
 * EMM, ECM, ESM and EPS Security context definitions
 */
typedef struct{
    uint64_t    imsi;
    emm_state_t state;
    uint8_t     procedure_transaction_id;
    uint8_t     attach_type;
    struct in_addr ue_ip;
    srslte::gtpc_f_teid_ie sgw_ctrl_fteid;
} emm_ctx_t;

typedef struct{
  ecm_state_t state;
  uint32_t enb_ue_s1ap_id;
  uint32_t mme_ue_s1ap_id;
  struct   sctp_sndrcvinfo enb_sri;
  bool eit;
} ecm_ctx_t;

typedef struct{
  uint8_t erab_id;
  esm_state_t state;
  uint8_t qci;
  srslte::gtpc_f_teid_ie enb_fteid;
  srslte::gtpc_f_teid_ie sgw_s1u_fteid;
  srslte::gtpc_pdn_address_allocation_ie pdn_addr_alloc;
} esm_ctx_t;

typedef struct{
  uint8_t  eksi;
  uint8_t  k_asme[32];
  uint8_t  autn[16];
  uint8_t  rand[16];
  uint8_t  xres[16]; //minimum 6, maximum 16
  uint32_t dl_nas_count;
  uint32_t ul_nas_count;
  srslte::CIPHERING_ALGORITHM_ID_ENUM cipher_algo;
  srslte::INTEGRITY_ALGORITHM_ID_ENUM integ_algo;
  uint8_t k_nas_enc[32];
  uint8_t k_nas_int[32];
  uint8_t k_enb[32];
  LIBLTE_MME_UE_NETWORK_CAPABILITY_STRUCT ue_network_cap;
  bool ms_network_cap_present;
  LIBLTE_MME_MS_NETWORK_CAPABILITY_STRUCT ms_network_cap;
  LIBLTE_MME_EPS_MOBILE_ID_GUTI_STRUCT guti;
} sec_ctx_t;

/*
 * NAS Initialization Arguments
 */
typedef struct {
  uint16_t mcc;
  uint16_t mnc;
  uint8_t  mme_code;
  uint16_t mme_group;
  uint16_t tac;
  std::string apn;
  std::string dns;
} nas_init_t;

class nas
{
public:
  nas();
  void init(nas_init_t args,
            s1ap_interface_nas *s1ap,
            gtpc_interface_nas *gtpc,
            hss_interface_nas *hss,
            srslte::log *nas_log);

  /***********************
   * Initial UE messages *
   ***********************/
  //Attach request messages
  static bool handle_attach_request( uint32_t enb_ue_s1ap_id,
                                     struct sctp_sndrcvinfo *enb_sri,
                                     srslte::byte_buffer_t *nas_rx,
                                     nas_init_t args,
                                     s1ap_interface_nas *s1ap,
                                     gtpc_interface_nas *gtpc,
                                     hss_interface_nas  *hss,
                                     srslte::log        *nas_log);

  static bool handle_imsi_attach_request_unknown_ue( uint32_t enb_ue_s1ap_id,
                                                     struct sctp_sndrcvinfo *enb_sri,
                                                     const LIBLTE_MME_ATTACH_REQUEST_MSG_STRUCT &attach_req,
                                                     const LIBLTE_MME_PDN_CONNECTIVITY_REQUEST_MSG_STRUCT &pdn_con_req,
                                                     nas_init_t args,
                                                     s1ap_interface_nas *s1ap,
                                                     gtpc_interface_nas *gtpc,
                                                     hss_interface_nas  *hss,
                                                     srslte::log        *nas_log);

  static bool handle_imsi_attach_request_known_ue( nas *nas_ctx,
                                                   uint32_t enb_ue_s1ap_id,
                                                   struct sctp_sndrcvinfo *enb_sri,
                                                   const LIBLTE_MME_ATTACH_REQUEST_MSG_STRUCT &attach_req,
                                                   const LIBLTE_MME_PDN_CONNECTIVITY_REQUEST_MSG_STRUCT &pdn_con_req,
                                                   srslte::byte_buffer_t *nas_rx,
                                                   nas_init_t args,
                                                   s1ap_interface_nas *s1ap,
                                                   gtpc_interface_nas *gtpc,
                                                   hss_interface_nas  *hss,
                                                   srslte::log        *nas_log);


  static bool handle_guti_attach_request_unknown_ue( uint32_t enb_ue_s1ap_id,
                                                     struct sctp_sndrcvinfo *enb_sri,
                                                     const LIBLTE_MME_ATTACH_REQUEST_MSG_STRUCT &attach_req,
                                                     const LIBLTE_MME_PDN_CONNECTIVITY_REQUEST_MSG_STRUCT &pdn_con_req,
                                                     nas_init_t args,
                                                     s1ap_interface_nas *s1ap,
                                                     gtpc_interface_nas *gtpc,
                                                     hss_interface_nas  *hss,
                                                     srslte::log        *nas_log);

  static bool handle_guti_attach_request_known_ue( nas *nas_ctx,
                                                   uint32_t enb_ue_s1ap_id,
                                                   struct sctp_sndrcvinfo *enb_sri,
                                                   const LIBLTE_MME_ATTACH_REQUEST_MSG_STRUCT &attach_req,
                                                   const LIBLTE_MME_PDN_CONNECTIVITY_REQUEST_MSG_STRUCT &pdn_con_req,
                                                   srslte::byte_buffer_t *nas_rx,
                                                   nas_init_t args,
                                                   s1ap_interface_nas *s1ap,
                                                   gtpc_interface_nas *gtpc,
                                                   hss_interface_nas  *hss,
                                                   srslte::log        *nas_log);
  //Service request messages
  static bool handle_service_request( uint32_t m_tmsi,
                                      uint32_t enb_ue_s1ap_id,
                                      struct sctp_sndrcvinfo *enb_sri,
                                      srslte::byte_buffer_t *nas_rx,
                                      nas_init_t args,
                                      s1ap_interface_nas *s1ap,
                                      gtpc_interface_nas *gtpc,
                                      hss_interface_nas  *hss,
                                      srslte::log        *nas_log);

  //Dettach request messages
  static bool handle_detach_request(  uint32_t m_tmsi,
                                      uint32_t enb_ue_s1ap_id,
                                      struct sctp_sndrcvinfo *enb_sri,
                                      srslte::byte_buffer_t *nas_rx,
                                      nas_init_t args,
                                      s1ap_interface_nas *s1ap,
                                      gtpc_interface_nas *gtpc,
                                      hss_interface_nas  *hss,
                                      srslte::log        *nas_log);

  //Tracking area update request messages
  static bool handle_tracking_area_update_request( uint32_t m_tmsi,
                                                   uint32_t enb_ue_s1ap_id,
                                                   struct sctp_sndrcvinfo *enb_sri,
                                                   srslte::byte_buffer_t *nas_rx,
                                                   nas_init_t args,
                                                   s1ap_interface_nas *s1ap,
                                                   gtpc_interface_nas *gtpc,
                                                   hss_interface_nas  *hss,
                                                   srslte::log        *nas_log);

  /* Uplink NAS messages handling */
  bool handle_authentication_response      (srslte::byte_buffer_t *nas_rx);
  bool handle_security_mode_complete       (srslte::byte_buffer_t *nas_rx);
  bool handle_attach_complete              (srslte::byte_buffer_t *nas_rx);
  bool handle_esm_information_response     (srslte::byte_buffer_t *nas_rx);
  bool handle_identity_response            (srslte::byte_buffer_t *nas_rx);
  bool handle_tracking_area_update_request (srslte::byte_buffer_t *nas_rx);
  bool handle_authentication_failure       (srslte::byte_buffer_t *nas_rx);
  bool handle_detach_request           (srslte::byte_buffer_t *nas_rx);

  /* Downlink NAS messages packing */
  bool pack_authentication_request  (srslte::byte_buffer_t *nas_buffer);
  bool pack_authentication_reject   (srslte::byte_buffer_t *nas_buffer);
  bool pack_security_mode_command   (srslte::byte_buffer_t *nas_buffer);
  bool pack_esm_information_request (srslte::byte_buffer_t *nas_buffer);
  bool pack_identity_request        (srslte::byte_buffer_t *nas_buffer);
  bool pack_emm_information         (srslte::byte_buffer_t *nas_buffer);
  bool pack_service_reject          (srslte::byte_buffer_t *nas_buffer);
  bool pack_attach_accept           (srslte::byte_buffer_t *nas_buffer);

  /* Security functions */
  bool integrity_check       (srslte::byte_buffer_t *pdu);
  bool short_integrity_check (srslte::byte_buffer_t *pdu);

  /* UE Context */
  emm_ctx_t m_emm_ctx;
  ecm_ctx_t m_ecm_ctx;
  esm_ctx_t m_esm_ctx[MAX_ERABS_PER_UE];
  sec_ctx_t m_sec_ctx;

private:
  srslte::byte_buffer_pool   *m_pool;
  srslte::log                *m_nas_log;
  gtpc_interface_nas         *m_gtpc;
  s1ap_interface_nas         *m_s1ap;
  hss_interface_nas          *m_hss;

  uint16_t                    m_mcc;
  uint16_t                    m_mnc;
  uint16_t                    m_mme_group;
  uint16_t                    m_mme_code;
  uint16_t                    m_tac;
  std::string                 m_apn;
  std::string                 m_dns;
};

}//namespace
#endif // SRSEPC_NAS_H