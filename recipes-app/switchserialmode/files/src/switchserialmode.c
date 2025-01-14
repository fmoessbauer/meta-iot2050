/*
 * Copyright (c) Siemens AG, 2019-2022
 *
 * Authors:
 *  Gao Nian <nian.gao@siemens.com>
 *  Chao Zeng <chao.zeng@siemens.com>
 *
 * This file is subject to the terms and conditions of the MIT License.  See
 * COPYING.MIT file in the top-level directory.
 */
#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>
#include <getopt.h>
#include <string.h>
#include <sys/types.h>
#include <fcntl.h>
#include <unistd.h>
#include "switchserialmode.h"

extern serial_ops_t ttyuart_ops;
extern serial_ops_t cp210x_ops;
extern transceiver_ops_t sp339e_ops;

static serial_ops_t *serial_ops = NULL;

boardType_e get_board_type(void)
{
    int32_t ret;
    const uint8_t *modelPath = "/proc/device-tree/model";
    uint8_t recvBuf[30] = {0};
    int32_t fd = open(modelPath, O_RDONLY);

    if (fd  < 0) {
       perror("Get tye error");
       return ERROR;
    }

    ret = read(fd, recvBuf, 30);
    if (ret < 0) {
        perror("Error");
    }
    close(fd);

    if (0 == strcmp(recvBuf,"SIMATIC IOT2050 Advanced"))
        return ADVANCED_BOARD_PG1;
    else if (strstr(recvBuf,"Advanced"))
        return ADVANCED_BOARD;

    return BASIC_BOARD;
}

static void usage(void)
{
    fprintf(stdout,
        " -h, --help                     : display help information\n"
        " -m, --mode                     : set serial work mode, the mode can be set 'rs232' 'rs422 'or 'rs485'.\n"
        " -d, --display                  : display the current mode\n"
        " -t, --terminator               : Terminate the RS422 or RS485 bus.\n"
        " -s, --setup u16                : set rs485-pin's setup time in rs485 mode.\n"
        " -o, --hold u16                 : set rs485-pin's hold time in rs485 mode.\n"
    );
}

static struct option serial_long_options[] = {
    {"help", no_argument, NULL, 'h'},
    {"mode", required_argument, NULL, 'm'},
    {"display", no_argument, NULL, 'd'},
    {"setup", required_argument, NULL, 's'},
    {"hold", required_argument, NULL, 'o'},
    {"terminator", no_argument, NULL, 't'},
    { NULL, 0, NULL, 0}
};

static void init_ops(void)
{
    if (get_board_type())
        serial_ops = &cp210x_ops;
    else
        serial_ops = &ttyuart_ops;

    serial_ops->transOps = &sp339e_ops;

    if (serial_ops->init(serial_ops->devName))
    {
        printf("Init error\n");
        exit(1);
    }
}

int main(int argc, char **argv)
{
    bool display_current_mode = false;
    uint8_t *mode = NULL;
    uint32_t setupTime = 0;
    uint32_t holdTime = 0;
    bool set_termination = false;
    uint8_t *options = "hm:ds:o:t";
    int32_t c;

    /*parse parameter*/
    while ((c = getopt_long(argc, argv, options,
                serial_long_options, NULL)) != EOF) {
    switch (c) {
        case 'h':
            usage();
            exit(1);
        case 'd':
            display_current_mode = true;
            break;
        case 'm':
            mode = strdup(optarg);
            break;
        case 's':
            setupTime = atoi(optarg);
            break;
        case 'o':
            holdTime = atoi(optarg);
            break;
        case 't':
            set_termination = true;
            break;
        default:
            printf("Operation not supported\n");
            exit(1);
        }
    }

    init_ops();

    if (display_current_mode) {
        serial_ops->getMode();
        goto release;
    }

    if (NULL != mode) {
        serial_ops->setMode(mode);
    }

    if (holdTime) {
        serial_ops->rs485HoldTime(holdTime);
    }

    if (setupTime) {
        serial_ops->rs485SetupTime(setupTime);
    }

    /* configure transceiver */
    if (NULL != mode)
        serial_ops->transOps->transceiver_switch_mode(mode);

    //default is disable terminate
    serial_ops->transOps->transceiver_set_termination(set_termination);

release:
    if(serial_ops->release)
        serial_ops->release();

    return SUCCESS;
}
