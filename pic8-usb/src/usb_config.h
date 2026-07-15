/*
 * M-Stack application config for pic8-usb.
 *
 * Private to this module -- included only by the vendored M-Stack sources
 * (third_party/m-stack/usb/src/usb.c, usb_cdc.c) and by pic8_usb.c itself.
 * Not a public pic8-usb header; callers only ever see pic8_usb.h.
 *
 * Endpoint layout matches M-Stack's own cdc_acm demo
 * (third_party/m-stack/apps/cdc_acm/usb_config.h): EP1 IN is the CDC
 * notification endpoint (required by the CDC-ACM interface descriptor,
 * unused by pic8_usb.c beyond being present -- see pic8-usb-plan.md,
 * "deliberately not building" serial-state notifications), EP2 IN/OUT is
 * the bulk data pipe pic8_usb_write/read actually move bytes through.
 */

#ifndef PIC8_USB_CONFIG_H__
#define PIC8_USB_CONFIG_H__

#define NUM_ENDPOINT_NUMBERS 2

/* Only 8, 16, 32 and 64 are supported for endpoint zero length. */
#define EP_0_LEN 8

#define EP_1_OUT_LEN 1
#define EP_1_IN_LEN 10

#define EP_2_LEN 64
#define EP_2_OUT_LEN EP_2_LEN
#define EP_2_IN_LEN EP_2_LEN

#define NUMBER_OF_CONFIGURATIONS 1

#define PPB_MODE PPB_NONE

/* Polling, not interrupt-driven: pic8_usb_service() is called from the
 * caller's main loop or a pic8-taskmgr task -- see pic8-usb-plan.md,
 * "Servicing cadence". Leave USB_USE_INTERRUPTS undefined (M-Stack's demo
 * defines it by default and drives usb_service() from a high-priority ISR
 * instead; this module deliberately does not build that second mode -- see
 * the plan doc for why). */

/* Objects from usb_descriptors.c */
#define USB_DEVICE_DESCRIPTOR this_device_descriptor
#define USB_CONFIG_DESCRIPTOR_MAP usb_application_config_descs
#define USB_STRING_DESCRIPTOR_FUNC usb_application_get_string

/* Callbacks from usb.c -- implemented in pic8_usb.c under pic8_usb_* names
 * (not app_*, so this module doesn't collide with a firmware's own
 * app-level callbacks if something else in the same build also uses
 * M-Stack's app_* naming convention). */
#define SET_CONFIGURATION_CALLBACK         pic8_usb_set_configuration_cb
#define GET_DEVICE_STATUS_CALLBACK         pic8_usb_get_device_status_cb
#define ENDPOINT_HALT_CALLBACK             pic8_usb_endpoint_halt_cb
#define SET_INTERFACE_CALLBACK             pic8_usb_set_interface_cb
#define GET_INTERFACE_CALLBACK             pic8_usb_get_interface_cb
#define OUT_TRANSACTION_CALLBACK           pic8_usb_out_transaction_cb
#define IN_TRANSACTION_COMPLETE_CALLBACK   pic8_usb_in_transaction_complete_cb
#define UNKNOWN_SETUP_REQUEST_CALLBACK     pic8_usb_unknown_setup_request_cb
#define UNKNOWN_GET_DESCRIPTOR_CALLBACK    pic8_usb_unknown_get_descriptor_cb
#define START_OF_FRAME_CALLBACK            pic8_usb_start_of_frame_cb
#define USB_RESET_CALLBACK                 pic8_usb_reset_cb

/* CDC Configuration functions. See usb_cdc.h for documentation. */
#define CDC_SEND_ENCAPSULATED_COMMAND_CALLBACK pic8_usb_cdc_send_encapsulated_command_cb
#define CDC_GET_ENCAPSULATED_RESPONSE_CALLBACK pic8_usb_cdc_get_encapsulated_response_cb
#define CDC_SET_COMM_FEATURE_CALLBACK          pic8_usb_cdc_set_comm_feature_cb
#define CDC_CLEAR_COMM_FEATURE_CALLBACK        pic8_usb_cdc_clear_comm_feature_cb
#define CDC_GET_COMM_FEATURE_CALLBACK          pic8_usb_cdc_get_comm_feature_cb
#define CDC_SET_LINE_CODING_CALLBACK           pic8_usb_cdc_set_line_coding_cb
#define CDC_GET_LINE_CODING_CALLBACK           pic8_usb_cdc_get_line_coding_cb
#define CDC_SET_CONTROL_LINE_STATE_CALLBACK    pic8_usb_cdc_set_control_line_state_cb
#define CDC_SEND_BREAK_CALLBACK                pic8_usb_cdc_send_break_cb

#endif /* PIC8_USB_CONFIG_H__ */
