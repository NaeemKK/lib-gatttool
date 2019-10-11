# lib-gatttool

It is gatttool (Bluez) based library. It has support for handling multiple devices.
Currently connect, disconnect, read, write and notifications are supported.

## Need
    *BlueZ to use below internal libraries.
        lib/.libs/libbluetooth-internal.a
        src/.libs/libshared-glib.a
    *GLib
    *readline
## Usage
	
All function returns NULL in case of no error. If any issue is occured, a message is returned.  
	
1.  call 
```
const char *initialize_libgatttool(GThread *)
```    
It is recommended to pass Gthread object reference to this function to be 
able to monitor status of library. libraray hanldes blocked main loop call in a thread to
handle the events. g_thread_join can be called to on this object to determine if events 
are still handled by library or not. ToDo: propse a mechanism to handle such situation or 
leave it on user to handle main loop.
	
2. call 
```
const char *connect_dev(conn_handle_t **conn_handle, const char *interface, const char *dst,
		const char *dst_type, int mtu, int psm, const char *sec_level,dev_connect_cb cb,
		void *user_data);
```	
to connect to a remote ble device. 
disconnection handler: Set your disconnection handler by accessing conn_handler_t data 
memeber "disconn_handler" and pass conn_handle_t pointer reference. conn_handler_t is used 
for all functionality onwards to communicate with device. 
Handler returns the address fo device which gets disconnected 
```
void (*disconnect_cb)(char *dst_addr);
```
connection callback: when device gets connected by calling "connect_dev", user provided 
callback is called. It's type should be 
```
void (*dev_connect_cb)(conn_handle_t *conn_handle, gpointer user_data);
```
It returns the user provided conn_handler and any user data passed to  "connect_dev" function.
	
3. Call 
```
int uuid_to_handle(char *uuid, cb)
```
to get handle against user provided uuid. A callback should also be registered	 which returns 
the uuid and its handler to the user. Handler should be of type
```
void (*uuid_to_handle_cb) (char *uuid,int handle);
```
it return -1 in case of error;

4. Call 
```
register_notification_handler(conn_handle_t *conn_handle,int handle,GAttribNotifyFunc cb, gpointer user_data)
```
to register for notifications.
Callback cb: 
```
void (*GAttribNotifyFunc)(const guint8 *pdu, guint16 len,gpointer user_data);
```
Register callback of this type to get notified about incomming notifications.
pdu[0] conatins type:
ATT_OP_HANDLE_NOTIFY -- notifications
ATT_OP_HANDLE_IND -- not supported and won't be reported 

pdu[1] and pdu[2] contains the handle

pdu[3] onwards contains the data

5. Call  
```
char_read(conn_handle_t *conn_handle , int handle, cb,void *user_data);
```
cb:
```
void (*char_read_by_handle_cb)(guint8 status, const guint8 *pdu,
            guint16 plen, gpointer user_data);
```
6. Call
```
char_write(conn_handle_t *conn_handle , int handle, uint8_t *value, size_t plen,bool response_needed,
   char_write_req_cb cb, void *user_data)
``` 
response_needed is true if write is with response, otherwise set it to false. When response is needed,
follwoinf callback is to be registered 	
```
void (*char_write_req_cb)(guint8 status, const guint8 *pdu, guint16 plen,
   gpointer user_data);
```					
	 
## ToDO
1. Run and Test
2. Add functions to read and write through uuid
3. Add missing functionlaity after test (if required)
