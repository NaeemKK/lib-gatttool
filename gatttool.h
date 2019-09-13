
#ifndef __GATTOOL_H_
#define __GATTOOL_H_

/* @Reference
 *
 *  BlueZ - Bluetooth protocol stack for Linux
 *
 *  Copyright (C) 2011  Nokia Corporation
 */

typedef void (*char_write_req_cb)(guint8 status, const guint8 *pdu, guint16 plen,
		gpointer user_data);

typedef void (*char_read_by_uuid_cb)(guint8 status, const guint8 *pdu,
					guint16 plen, gpointer user_data);

const char * initialize(conn_params_t *params, const char *error);
const char * interactive(const char *interface, const char *dst,
		const char *dst_type, int mtu, int psm, const char *sec_level);


typedef struct conn_params_s
{
	char *src;
	char *dst;
	char *dst_type;
	char *sec_level;
	int mtu;
	int psm;
}conn_params_t;

#endif
