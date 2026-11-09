/* SPDX-License-Identifier: GPL-2.0-or-later
 * Regression test for usb_disconnect() / rx_callback() teardown when
 * libusb_submit_transfer or libusb_cancel_transfer fails after device removal.
 */
#include "usb.c"
#include <assert.h>
#include <stdio.h>

static int submit_result = LIBUSB_ERROR_NO_DEVICE;
static int cancel_result = LIBUSB_ERROR_NOT_FOUND;
static int event_loop_spins;
static int freed_transfers;
static int removed_devices;

void usbmuxd_log(enum loglevel level, const char *fmt, ...) { (void)level; (void)fmt; }
void device_data_input(struct usb_device *usbdev, unsigned char *buffer, uint32_t length)
{ (void)usbdev; (void)buffer; (void)length; }
void device_remove(struct usb_device *usbdev) { (void)usbdev; removed_devices++; }

const char * LIBUSB_CALL libusb_error_name(int errcode) { (void)errcode; return "stub"; }
int LIBUSB_CALL libusb_submit_transfer(struct libusb_transfer *transfer)
{ (void)transfer; return submit_result; }
int LIBUSB_CALL libusb_cancel_transfer(struct libusb_transfer *transfer)
{ (void)transfer; return cancel_result; }
void LIBUSB_CALL libusb_free_transfer(struct libusb_transfer *transfer)
{ free(transfer); freed_transfers++; }
int LIBUSB_CALL libusb_handle_events_timeout(libusb_context *ctx, struct timeval *tv)
{ (void)ctx; (void)tv; event_loop_spins++; return 0; }
int LIBUSB_CALL libusb_release_interface(libusb_device_handle *dev_handle, int interface_number)
{ (void)dev_handle; (void)interface_number; return 0; }
void LIBUSB_CALL libusb_close(libusb_device_handle *dev_handle) { (void)dev_handle; }

static struct libusb_transfer *make_xfer(struct usb_device *dev)
{
    struct libusb_transfer *xfer = calloc(1, sizeof(*xfer));
    xfer->buffer = malloc(64);
    xfer->actual_length = 16;
    xfer->status = LIBUSB_TRANSFER_COMPLETED;
    xfer->user_data = dev;
    return xfer;
}

int main(void)
{
    int dummy_handle = 1;
    collection_init(&device_list);

    /* Case 1: rx_callback completes just as device disappears; resubmit fails.
     * The unsubmitted xfer must be removed immediately and mark dev dead so
     * reap_dead_devices() does not spin in libusb_handle_events_timeout().
     */
    struct usb_device *dev1 = calloc(1, sizeof(*dev1));
    dev1->dev = (libusb_device_handle *)&dummy_handle;
    dev1->alive = 1;
    collection_init(&dev1->rx_xfers);
    collection_init(&dev1->tx_xfers);
    collection_add(&device_list, dev1);

    struct libusb_transfer *rx1 = make_xfer(dev1);
    collection_add(&dev1->rx_xfers, rx1);
    rx_callback(rx1);
    assert(dev1->alive == 0);
    assert(collection_count(&dev1->rx_xfers) == 0);

    reap_dead_devices();
    assert(collection_count(&device_list) == 0);
    assert(event_loop_spins == 0);
    assert(removed_devices == 1);

    /* Case 2: usb_disconnect sees stranded rx/tx xfers whose cancel returns
     * LIBUSB_ERROR_NOT_FOUND; it must reap them directly without spinning.
     */
    struct usb_device *dev2 = calloc(1, sizeof(*dev2));
    dev2->dev = (libusb_device_handle *)&dummy_handle;
    dev2->alive = 0;
    collection_init(&dev2->rx_xfers);
    collection_init(&dev2->tx_xfers);
    collection_add(&device_list, dev2);
    collection_add(&dev2->rx_xfers, make_xfer(dev2));
    collection_add(&dev2->tx_xfers, make_xfer(dev2));

    reap_dead_devices();
    assert(collection_count(&device_list) == 0);
    assert(event_loop_spins == 0);
    assert(freed_transfers == 3);
    assert(removed_devices == 2);

    collection_free(&device_list);
    puts("usb-disconnect teardown: failed resubmit/cancel reaped without spinning");
    return 0;
}
