/* SPDX-License-Identifier: GPL-2.0-or-later
 * Compile against the actual Ubuntu usbmuxd source, not a copy of its logic.
 * USB/client boundaries are stubs; no daemon or hardware is accessed.
 */
#include "device.c"
#include <assert.h>
#include <stdio.h>

static int close_calls;
static int notify_calls;
void usbmuxd_log(enum loglevel level, const char *fmt, ...) { (void)level; (void)fmt; }
int usb_send(struct usb_device *dev, const unsigned char *buf, int length)
{ (void)dev; (void)buf; return length; }
void client_close(struct mux_client *client) { (void)client; close_calls++; }
int client_notify_connect(struct mux_client *client, enum usbmuxd_result result)
{ (void)client; (void)result; notify_calls++; return 0; }
int client_write(struct mux_client *client, void *buffer, uint32_t len)
{ (void)client; (void)buffer; return (int)len; }

int main(void)
{
    collection_init(&device_list);
    struct mux_device device = { .id = 7, .state = MUXDEV_DEAD };
    collection_init(&device.connections);
    collection_add(&device_list, &device);
    int client_token;
    struct mux_client *client = (struct mux_client *)&client_token;

    /* A socket can die after SYN/ACK changes the connection to CONNECTED,
     * while its client is still CONNECTING2 waiting to flush the response.
     * client_close() then calls device_abort_connect(). A callback to
     * client_close() here would free that same client twice.
     */
    enum mux_conn_state states[] = { CONN_CONNECTED, CONN_CONNECTING,
                                    CONN_REFUSED, CONN_DYING };
    for (unsigned i = 0; i < sizeof(states) / sizeof(states[0]); i++) {
        struct mux_connection *conn = calloc(1, sizeof(*conn));
        conn->dev = &device;
        conn->client = client;
        conn->state = states[i];
        collection_add(&device.connections, conn);
        device_abort_connect(device.id, client);
        assert(collection_count(&device.connections) == 0);
    }
    collection_free(&device.connections);
    collection_free(&device_list);
    if (close_calls || notify_calls) {
        fprintf(stderr, "closing client received %d close and %d notify callbacks\n",
                close_calls, notify_calls);
        return 1;
    }
    puts("abort-connect teardown: no reentrant client callbacks");
    return 0;
}
