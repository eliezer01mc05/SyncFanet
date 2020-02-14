/* Copyright (C) Uppsala University
 *
 * This file is distributed under the terms of the GNU general Public
 * License (GPL), see the file LICENSE
 *
 * Author: Erik Nordström, <erikn@it.uu.se>
 */
#ifndef _DSR_RREP_H
#define _DSR_RREP_H

#include "uavnet/routing/manet/dsr/dsr-uu/dsr.h"
#include "uavnet/routing/manet/dsr/dsr-uu/dsr-srt.h"
#include "uavnet/routing/manet/dsr/dsr-uu/dsr-opt.h"

#ifndef NO_GLOBALS

#define DSR_RREP_OPT_LEN(srt) (DSR_RREP_HDR_LEN + (srt->addrs.size()*DSR_ADDRESS_SIZE) + DSR_ADDRESS_SIZE)

namespace inet {

namespace inetmanet {
/* Length of source route is length of option, minus reserved/flags field minus
 * the last source route hop (which is the destination) */
#define DSR_RREP_ADDRS_LEN(rrep_opt) ((dynamic_cast<dsr_rrep_opt*>(rrep_opt)->addrs.size()-1)*DSR_ADDRESS_SIZE)
} // namespace inetmanet

} // namespace inet

#endif              /* NO_GLOBALS */

#ifndef NO_DECLS

int dsr_rrep_opt_recv(struct dsr_pkt *dp, struct dsr_rrep_opt *rrep_opt);
int dsr_rrep_send(struct dsr_srt *srt, struct dsr_srt *srt_to_me);

void grat_rrep_tbl_timeout(void *data);
int grat_rrep_tbl_add(struct in_addr src, struct in_addr prev_hop);
int grat_rrep_tbl_find(struct in_addr src, struct in_addr prev_hop);
int grat_rrep_tbl_init(void);
void grat_rrep_tbl_cleanup(void);

#endif              /* NO_DECLS */

#endif              /* _DSR_RREP */
