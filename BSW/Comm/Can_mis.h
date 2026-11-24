#ifndef CAN_MIS_H
#define CAN_MIS_H

#include <stdint.h>
#include <Can_cfg.h>
#include <zephyr/canbus/isotp.h>


int32_t can_init_mis(struct isotp_recv_ctx *rctx, const struct device *can_dev, const struct isotp_msg_id *rx_addr, const struct isotp_msg_id *tx_addr, const struct isotp_fc_opts *opts);
void can_receive_data(struct isotp_recv_ctx *rctx); 
void can_send_data(struct isotp_send_ctx *sctx, const struct device *can_dev, const uint8_t *data, size_t len, const struct isotp_msg_id *tx_addr, const struct isotp_msg_id *rx_addr, isotp_tx_callback_t complete_cb, void *cb_arg); 

#endif



