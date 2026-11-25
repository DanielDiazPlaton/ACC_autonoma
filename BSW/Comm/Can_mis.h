#ifndef CAN_MIS_H
#define CAN_MIS_H

#include <stdint.h>
#include <stddef.h>
#include "Can_cfg.h"
#include <zephyr/drivers/can.h>
#include <zephyr/canbus/isotp.h>

int32_t can_init_mis(void);
void can_receive_data(void);
int32_t can_send_data(const uint8_t *data, size_t len);

#endif /* CAN_MIS_H */