
#ifndef __GATTOOL_H_
#define __GATTOOL_H_

/* @Reference
 *
 *  BlueZ - Bluetooth protocol stack for Linux
 *
 *  Copyright (C) 2011  Nokia Corporation
 */
#include "gatt_comm.h"

typedef (*disconnect_cb)(char *dst_addr);

typedef struct
{
	GIOChannel iochannel;
	GAttrib attrib;
	guint gsrc;
	char *dst_addr;
	disconnect_cb cb;
}conn_handle_t;

typedef void (*char_write_req_cb)(guint8 status, const guint8 *pdu, guint16 plen,
		gpointer user_data);

typedef void (*char_read_by_handle_cb)(guint8 status, const guint8 *pdu,
					guint16 plen, gpointer user_data);

const char * initialize(void);
const char * connect(conn_handle_t **conn_handle, const char *interface, const char *dst,
		const char *dst_type, int mtu, int psm, const char *sec_level);
int uuid_to_handle(char *puuid,uuid_to_handle_cb cb);
const char * char_write(conn_handle_t *conn_handle , int handle, uint8_t *value, size_t plen,bool response_needed,
		char_write_req_cb cb, void *user_data);
const char * char_read(conn_handle_t *conn_handle , int handle,char_read_by_handle_cb cb,
		void *user_data);
int register_notification_handler(conn_handle_t *conn_handle,int handle,GAttribNotifyFunc func, gpointer user_data);
void disconnect(conn_handle_t *conn_handle);

#endif
