#ifndef ERROR_H
#define ERROR_H

#ifdef ERROR_FILE
volatile int panic;
#else
extern volatile int panic;
#endif

/*int get_panic ();
void set_panic (int level);*/
void myperror (char *msg);
void myerror (int err, char *msg);
void mygaierror (int err, char *msg);
void myserror (char *msg);

#endif
