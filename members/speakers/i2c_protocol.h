#pragma once

//i2c address
#define I2C_ADDR 3

//i2c protocol
#define CMD_LENGTH 10
#define CMD_BUFF_LEN (CMD_LENGTH + 1)
char cmdstr[CMD_BUFF_LEN] = "P#SSS@AAAA";

// "P#SSS@AAAA" - P: P (play), SSS: song #, A: amp. (x 1000)
// "SXXXXXXXXX" - S: S (stop)
