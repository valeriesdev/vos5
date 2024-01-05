#ifndef STRINGS_H
#define STRINGS_H

char* int_to_ascii(int n);
char* hex_to_ascii(int n);
void reverse(char s[]);
int strlen(char s[]);
void backspace(char s[]);
void append(char s[], char n);
int strcmp(char s1[], char s2[]);
char **str_split(char *in, char delim);
int8_t character_exists(char char_to_find, char* string_to_search);

#endif
