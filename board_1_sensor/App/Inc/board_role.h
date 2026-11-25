#ifndef BOARD_ROLE_H
#define BOARD_ROLE_H
#define BOARD_STANDALONE 0
#define BOARD_SENSOR 1
#define BOARD_ACTUATOR 2
/* The two distributable projects replace this default in their own header. */
#ifndef BOARD_ROLE
#define BOARD_ROLE BOARD_SENSOR
#endif
#if BOARD_ROLE < BOARD_STANDALONE || BOARD_ROLE > BOARD_ACTUATOR
#error Invalid BOARD_ROLE
#endif
#endif
