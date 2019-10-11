/*
 *
 *  Author: Naeem Khan
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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <string.h>
#include <stdlib.h>
#include <stdarg.h>
#include <errno.h>
#include <stdio.h>
#include <unistd.h>
#include <signal.h>
#include <sys/signalfd.h>
#include <glib.h>

#include "lib/bluetooth.h"
#include "lib/sdp.h"
#include "lib/uuid.h"

#include "src/shared/util.h"
#include "btio/btio.h"
#include "att.h"
#include "gattrib.h"
#include "gatt.h"
#include "gatttool.h"

static GIOChannel *iochannel = NULL;
static GAttrib *attrib = NULL;
static GMainLoop *event_loop;
static GThread *thread;

//
//static void events_handler(const uint8_t *pdu, uint16_t len, gpointer user_data)
//{
//	uint8_t *opdu;
//	uint16_t handle, i, olen;
//	size_t plen;
//	GString *s;
//
//	handle = get_le16(&pdu[1]);
//
//	switch (pdu[0]) {
//	case ATT_OP_HANDLE_NOTIFY:
//		s = g_string_new(NULL);
//		g_string_printf(s, "Notification handle = 0x%04x value: ",
//									handle);
//		break;
//	case ATT_OP_HANDLE_IND:
//		s = g_string_new(NULL);
//		g_string_printf(s, "Indication   handle = 0x%04x value: ",
//									handle);
//		break;
//	default:
//		error("Invalid opcode\n");
//		return;
//	}
//
//	for (i = 3; i < len; i++)
//		g_string_append_printf(s, "%02x ", pdu[i]);
//
//	rl_printf("%s\n", s->str);
//	g_string_free(s, TRUE);
//
//	if (pdu[0] == ATT_OP_HANDLE_NOTIFY)
//		return;
//
//	opdu = g_attrib_get_buffer(attrib, &plen);
//	olen = enc_confirmation(opdu, plen);
//
//	if (olen > 0)
//		g_attrib_send(attrib, 0, opdu, olen, NULL, NULL, NULL);
//}

static void conn_cb(GIOChannel *io, GError *err, gpointer user_data)
{
	uint16_t mtu;
	uint16_t cid;

	data_connect_cb_t *data = user_data;

	if (err) {
		g_io_channel_shutdown(data->conn_handle->iochannel, FALSE, NULL);
		g_io_channel_unref(data->conn_handle->iochannel);
		data->conn_handle->iochannel = NULL;

		g_source_remove(data->conn_handle->gsrc);

		g_free(data->conn_handle->dst_addr);

		g_free(data->conn_handle);
		data->conn_handle = NULL;
		data->cb(NULL,data->user_data);
		g_free(data);
		return;
	}

	bt_io_get(io, &err, BT_IO_OPT_IMTU, &mtu,
				BT_IO_OPT_CID, &cid, BT_IO_OPT_INVALID);

	if (err) {
		g_printerr("Can't detect MTU, using default: %s", err->message);
		g_error_free(err);
		mtu = ATT_DEFAULT_LE_MTU;
	}

	if (cid == ATT_CID)
		mtu = ATT_DEFAULT_LE_MTU;

	data->conn_handle->attrib = g_attrib_new(iochannel, mtu, false);
	data->cb(data->conn_handle,data->user_data);
	g_free(data);
}

int register_notification_handler(conn_handle_t *conn_handle,int handle,GAttribNotifyFunc func, gpointer user_data)
{
	return g_attrib_register(conn_handle->attrib, ATT_OP_HANDLE_NOTIFY, handle,
			func, user_data, NULL);
}

static void disconnect_io(void *user_data)
{
	conn_handle_t *data = user_data;

	if(! data)
		return;

	g_attrib_unref(data->attrib);
	data->attrib = NULL;

	g_io_channel_shutdown(data->iochannel, FALSE, NULL);
	g_io_channel_unref(data->iochannel);
	data->iochannel = NULL;

	g_source_remove(data->gsrc);

	data->discon_handler(data->dst_addr);

	g_free(data->dst_addr);
	g_free(data);
	data = NULL;
}

static gboolean channel_watcher(GIOChannel *chan, GIOCondition cond,
				gpointer user_data)
{
	disconnect_io(user_data);

	return FALSE;
}

void disconnect(conn_handle_t *conn_handle)
{
	disconnect_io(conn_handle);
}

const char * char_read(conn_handle_t *conn_handle , int handle,char_read_by_handle_cb cb,
		void *user_data)
{
	/* ToDo: Bluez tool does not return status but check the return type of
	 * functions inside it and return status to the user accordingly about
	 * success and failure.
	 */
	gatt_read_char(conn_handle->attrib, handle, cb, user_data);

	return NULL;
}

const char * char_write(conn_handle_t *conn_handle , int handle, uint8_t *value, size_t plen,bool response_needed,
		char_write_req_cb cb, void *user_data)
{
	if (value == NULL)
	{
		return "No data is provided";
	}

	if (handle <= 0)
	{
		return "A valid handle is required";
	}

	if (plen == 0)
	{
		return "Invalid length";
	}

	if (response_needed == true)
		gatt_write_char(conn_handle->attrib, handle, value, plen,
				cb, user_data);
	else
		gatt_write_cmd(conn_handle->attrib, handle, value, plen, NULL, NULL);

	g_free(value);

	return NULL;
}

static void get_handle_cb(uint8_t status, GSList *characteristics, void *user_data)
{
	GSList *l;

	data_to_report_handle_t *data = user_data;

	if (status)
	{
		//Discover all characteristics failed
		data->cb(NULL,-1);
		goto free_mem;
	}

	for (l = characteristics; l; l = l->next)
	{
		struct gatt_char *chars = l->data;
		if(!strcmp(chars->uuid,data->uuid))
		{
			data->cb(chars->uuid,chars->value_handle);
		}
	}
free_mem:
	g_free(data);
}

int uuid_to_handle(char *puuid,uuid_to_handle_cb cb)
{
	int start = 0x0001;
	int end = 0xffff;

	bt_uuid_t uuid;

	if (bt_string_to_uuid(&uuid, puuid) < 0)
	{
		return -1;
	}

	data_to_report_handle_t *user_data = g_malloc(sizeof(data_to_report_handle_t));
	memcpy(user_data->uuid,puuid,CHARACHTERISTIC_MAX_LEN);
	user_data->cb = cb;

	gatt_discover_char(attrib, start, end, &uuid, get_handle_cb, user_data);
	return 0;
}

const char * connect_dev(conn_handle_t **conn_handle, const char *interface, const char *dst,
		const char *dst_type, int mtu, int psm, const char *sec_level,dev_connect_cb cb,void *user_data)
{
	GError *gerr = NULL;

	gchar *opt_interface = NULL;
	gchar *opt_dst_type = NULL;
	gchar *opt_sec_level = NULL;

	enum err_codes
	{
		NONE=0,
		DST_MISSING,
		MEM_ISSUE,
		CONN_ISSUE
	}err_code;

	err_code = NONE;

	if(sec_level)
		opt_sec_level = g_strdup(sec_level);
	else
		opt_sec_level = g_strdup("low");

	if(interface)
		opt_interface = g_strdup(interface);
	else
		opt_interface = g_strdup("hci0");

	if(!dst)
	{
		err_code = DST_MISSING;
		goto err;
	}

	if(dst_type)
		opt_dst_type = g_strdup(dst_type);
	else
		opt_dst_type = g_strdup("public");

	*conn_handle = g_malloc(sizeof(conn_handle_t));
	if(*conn_handle == NULL)
	{
		err_code = MEM_ISSUE;
		goto err;
	}

	(*conn_handle)->dst_addr = g_strdup(dst);

	extern GIOChannel *mod_gatt_connect(const char *src, const char *dst,
			const char *dst_type, const char *sec_level,
			int psm, int mtu, BtIOConnect connect_cb,gpointer user_data,
			GError **gerr);

	data_connect_cb_t *u_data = g_malloc(sizeof(data_connect_cb_t));
	if(!u_data)
	{
		err_code = MEM_ISSUE;
		goto err;
	}

	u_data->cb = cb;
	u_data->conn_handle = *conn_handle;
	u_data->user_data = user_data;

	(*conn_handle)->iochannel = mod_gatt_connect(opt_interface, dst, opt_dst_type, opt_sec_level,
					psm, mtu, conn_cb,u_data, &gerr);
	if ((*conn_handle)->iochannel == NULL)
	{
		g_error_free(gerr);
		err_code = CONN_ISSUE;
		goto err;
	} else
		(*conn_handle)->gsrc = g_io_add_watch((*conn_handle)->iochannel, G_IO_HUP, channel_watcher, *conn_handle);

	g_free(opt_interface);
	g_free(opt_dst_type);
	g_free(opt_sec_level);

	return NULL;

err:
	g_free(opt_interface);
	g_free(opt_dst_type);
	g_free(opt_sec_level);

	switch(err_code)
	{
		case NONE:
			return NULL;
		case DST_MISSING:
			return "User provided option issue";
		case MEM_ISSUE:
			return "Internal error";
		case CONN_ISSUE:
			g_free(*conn_handle);
			*conn_handle = NULL;
			return "Device could not be connected";
	}

	return NULL; //avoid warning
}

static gpointer event_loop_th(gpointer data)
{
	event_loop = g_main_loop_new(NULL, FALSE);
	g_main_loop_run(event_loop);
	g_main_loop_unref(event_loop);

	return NULL;
}
const char * initialize_libgatttool(GThread **thread_mon)
{
	GError *error;

	thread = g_thread_try_new (NULL,event_loop_th,NULL,&error);
	if(!thread)
	{
		g_free(error);
		goto err;
	}
	if(thread_mon) *thread_mon = thread;

	return NULL;
err:
	return "Initialization failed due to internal error";

}
