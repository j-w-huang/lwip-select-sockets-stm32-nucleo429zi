/*
 * printf-stdarg.h
 *
 *  Created on: Sep 11, 2019
 *      Author: yuer
 */

#ifndef INC_MPRINTF_H_
#define INC_MPRINTF_H_

int msprintf(char *out, const char *format, ...);
int msnprintf( char *buf, unsigned int count, const char *format, ... );
int mprintf(const char *format, ...);


#endif /* INC_MPRINTF_H_ */
