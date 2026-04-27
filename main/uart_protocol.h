#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "control.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    THERMOWORK_UART_MSG_NONE = 0,
    THERMOWORK_UART_MSG_PROCESS_VALUES,
    THERMOWORK_UART_MSG_CONFIG,
    THERMOWORK_UART_MSG_COMMAND,
    THERMOWORK_UART_MSG_INVALID
} thermowork_uart_msg_type_t;

typedef struct {
    thermowork_uart_msg_type_t type;
    thermowork_process_values_t process_values;
    thermowork_control_config_t config;
    bool command_enable_outputs;
} thermowork_uart_message_t;

void thermowork_uart_protocol_init(void);
thermowork_uart_msg_type_t thermowork_uart_parse_line(const char *line, thermowork_uart_message_t *message);
int thermowork_uart_format_status(
    char *buffer,
    size_t buffer_size,
    const thermowork_control_output_t *output,
    const char *fault_string
);

#ifdef __cplusplus
}
#endif
