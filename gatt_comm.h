/*
 * gatt_comm.h
 *
 *  Author: Naeem Khan
 */

#ifndef GATT_COMM_H_
#define GATT_COMM_H_

#define CHARACHTERISTIC_MAX_LEN 36

typedef void (*uuid_to_handle_cb) (int handle);
typedef void (*dev_connect_cb)(GIOChannel *io, GError *err, gpointer user_data);

typedef struct
{
	char uuid[CHARACHTERISTIC_MAX_LEN+1];
	uuid_to_handle_cb cb;
}data_to_report_handle_t;

typedef struct
{
	conn_handle_t *conn_handle;
	void *user_data;
	dev_connect_cb cb;
}data_connect_cb_t;

const char * interactive(void);

#endif /* GATT_COMM_H_ */
