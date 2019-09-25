# lib-gatttool

It is gatttool (Bluez) based library. It has support for handling multiple devices. Currently connect, disconnect, read, 
write and notifications are supported.

## Need
    *BlueZ to use below internal libraries.
        lib/.libs/libbluetooth-internal.a
        src/.libs/libshared-glib.a
    *GLib
    *readline
    
## ToDO
1. Add usage docs
2. Run and Test
3. Add functions to read and write through uuid
4. Add missing functionlaity after test (if required)
