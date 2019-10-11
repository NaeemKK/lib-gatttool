

#ifndef __GATTOOL_H_
#define __GATTOOL_H_
/*
 *	Author: Naeem Khan
 *
 *  lib-gatttool - Bluez based library without using dbus interface
 *
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 *
 */

#include "gattrib.h"

#define CHARACHTERISTIC_MAX_LEN 36

typedef void (*disconnect_cb)(char *dst_addr);

typedef struct
{
	GIOChannel *iochannel;
	GAttrib *attrib;
	guint gsrc;
	char *dst_addr;
	disconnect_cb discon_handler;
}conn_handle_t;

typedef void (*uuid_to_handle_cb) (char *uuid,int handle);
typedef void (*dev_connect_cb)(conn_handle_t *conn_handle, gpointer user_data);
typedef void (*char_write_req_cb)(guint8 status, const guint8 *pdu, guint16 plen,
		gpointer user_data);
typedef void (*char_read_by_handle_cb)(guint8 status, const guint8 *pdu,
					guint16 plen, gpointer user_data);

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

const char * initialize_libgatttool(GThread **thread_mon);
const char * connect_dev(conn_handle_t **conn_handle, const char *interface, const char *dst,
		const char *dst_type, int mtu, int psm, const char *sec_level,dev_connect_cb cb,void *user_data);
int uuid_to_handle(char *puuid,uuid_to_handle_cb cb);
const char * char_write(conn_handle_t *conn_handle , int handle, uint8_t *value, size_t plen,bool response_needed,
		char_write_req_cb cb, void *user_data);
const char * char_read(conn_handle_t *conn_handle , int handle,char_read_by_handle_cb cb,
		void *user_data);
int register_notification_handler(conn_handle_t *conn_handle,int handle,GAttribNotifyFunc func, gpointer user_data);
void disconnect(conn_handle_t *conn_handle);

#endif
