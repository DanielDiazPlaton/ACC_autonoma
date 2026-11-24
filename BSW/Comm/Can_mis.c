#include <Can_mis.h>





const struct isotp_msg_id rx_addr_8_0 = {
	.std_id = 0x80,
#ifdef CONFIG_SAMPLE_CAN_FD_MODE
	.flags = ISOTP_MSG_FDF | ISOTP_MSG_BRS,
#endif
};
const struct isotp_msg_id tx_addr_8_0 = {
	.std_id = 0x180,
#ifdef CONFIG_SAMPLE_CAN_FD_MODE
	.dl = 64,
	.flags = ISOTP_MSG_FDF | ISOTP_MSG_BRS,
#endif
};
const struct isotp_msg_id rx_addr_0_5 = {
	.std_id = 0x01,
#ifdef CONFIG_SAMPLE_CAN_FD_MODE
	.flags = ISOTP_MSG_FDF | ISOTP_MSG_BRS,
#endif
};
const struct isotp_msg_id tx_addr_0_5 = {
	.std_id = 0x101,
#ifdef CONFIG_SAMPLE_CAN_FD_MODE
	.dl = 64,
	.flags = ISOTP_MSG_FDF | ISOTP_MSG_BRS,
#endif
};

const struct device *can_dev;
struct isotp_recv_ctx recv_ctx_8_0;
struct isotp_recv_ctx recv_ctx_0_5;

const char tx_data_large[] =
"========================================\n"
"|   ____  ___  ____       ____  ____   |\n"
"|  |_  _|/ __||    | ___ |_  _||  _ \\  |\n"
"|   _||_ \\__ \\| || | ___   ||  | ___/  |\n"
"|  |____||___/|____|       ||  |_|     |\n"
"========================================\n";

const char tx_data_small[] = "This is the sample test for the short payload\n";

static struct isotp_send_ctx send_ctx_8_0;
static struct isotp_send_ctx send_ctx_0_5;
const struct isotp_fc_opts fc_opts_8_0 = {.bs = 8, .stmin = 0};
const struct isotp_fc_opts fc_opts_0_5 = {.bs = 0, .stmin = 5};

int32_t can_init_mis(struct isotp_recv_ctx *rctx, const struct device *can_dev, const struct isotp_msg_id *rx_addr, const struct isotp_msg_id *tx_addr, const struct isotp_fc_opts *opts)
{
    int32_t ret;

	ret = isotp_bind(&recv_ctx_8_0, can_dev,
			 &tx_addr_8_0, &rx_addr_8_0,
			 &fc_opts_8_0, K_FOREVER);
	if (ret != ISOTP_N_OK) {
		printk("Failed to bind to rx ID %d [%d]\n",
		       rx_addr_8_0.std_id, ret);
	}

    can_dev = DEVICE_DT_GET(DT_CHOSEN(zephyr_canbus));
    if (!device_is_ready(can_dev)) {
        printk("CAN: Device driver not ready.\n");
        ret = 0;
    }

    return ret;
}

void send_complette_cb(int error_nr, void *arg)
{
	ARG_UNUSED(arg);
	printk("TX complete cb [%d]\n", error_nr);
}


void can_receive_data(struct isotp_recv_ctx *rctx)
{
    int rem_len, received_len;
	struct net_buf *buf;
    received_len = 0;
    do {
        rem_len = isotp_recv_net(&recv_ctx_8_0, &buf,
                        K_MSEC(2000));
        if (rem_len < 0) {
            printk("Receiving error [%d]\n", rem_len);
            break;
        }

        while (buf != NULL) {
            received_len += buf->len;
            printk("%.*s", buf->len, buf->data);
            buf = net_buf_frag_del(NULL, buf);
        }
    } while (rem_len);
    printk("Got %d bytes in total\n", received_len);
}


void can_send_data(struct isotp_send_ctx *sctx, const struct device *can_dev, const uint8_t *data, size_t len, const struct isotp_msg_id *tx_addr, const struct isotp_msg_id *rx_addr, isotp_tx_callback_t complete_cb, void *cb_arg)
{
    int32_t ret;

    printk("Start sending data\n");

    k_msleep(1000);
    ret = isotp_send(&send_ctx_0_5, can_dev,
                tx_data_small, sizeof(tx_data_small),
                &tx_addr_0_5, &rx_addr_0_5,
                send_complette_cb, NULL);
    if (ret != ISOTP_N_OK) {
        printk("Error while sending data to ID %d [%d]\n",
                tx_addr_0_5.std_id, ret);
    }

    ret = isotp_send(&send_ctx_8_0, can_dev,
                tx_data_large, sizeof(tx_data_large),
                &tx_addr_8_0, &rx_addr_8_0, NULL, NULL);
    if (ret != ISOTP_N_OK) {
        printk("Error while sending data to ID %d [%d]\n",
                tx_addr_8_0.std_id, ret);
    }
}