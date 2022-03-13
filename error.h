#ifndef _ERROR_H_
#define _ERROR_H_

enum err_code {
  ELIB = 1,
  EREAD = 2,
};

/**
 * @brief Print error message corresponding to given error code
 * 
 * @param msg message to be printed
 * @param code error code to be interpreted
 */
void printerr(const char *msg, int code);

#endif // _ERROR_H_
