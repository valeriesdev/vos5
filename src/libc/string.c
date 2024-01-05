/**
 * @defgroup   STRING string
 * @ingroup    LIBC
 *
 * @brief      This file implements string functions.
 * @par
 * Most functions are either directly from or derived from Kernighan and Ritchie's The C Programming Language (2nd edition). If so, their source is labeled.
 * @author     Valerie Whitmire
 * @date       2023
 */
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include "drivers/screen.h"
#include "libc/string.h"
#include "libc/mem.h"
#include "libc/math.h"

/**
 * K&R, section 3.6
 * BROKEN! freeing the str causes a hard crash
 */
char* int_to_ascii(int n) {
    int i, sign;
    char* str = malloc(sizeof(char)* (logi(n, 10) > 1) ? logi(n, 10) : 1);

    if ((sign = n) < 0)           /* record sign */
        n = -n;                   /* make n positive */
    i = 0;
    do {                          /* generate digits in reverse order */
        str[i++] = n % 10 + '0';  /* get next digit */
    } while ((n /= 10) > 0);      /* delete it */
    if (sign < 0)
        str[i++] = '-';
    str[i] = '\0';
    reverse(str);
    return str;
}

/**
 * @brief      Converts an int to ascii hex format
 *
 * @param[in]  n     The integer to be converted
 *
 * @return     A char* with the ascii hex string
 * 
 * @todo       Completely rework function to operate in-place, if possible.
 * @note       Requires the use of malloc, so it cannot be used before the memory has been initialized.
 */
char* hex_to_ascii(int n) {
    char *str = malloc(sizeof(char)*12); // needs to be replaced with inplace
    str[0] = '\0';
    append(str, '0');
    append(str, 'x');

    const char * hex = "0123456789abcdef";
    uint8_t blankspace = 1;

    int32_t tmp;
    int i;
    for (i = 28; i >= 0; i -= 4) {
        tmp = (n >> i) & 0xF;
        if (tmp == 0 && blankspace) continue;
        blankspace = 0;
        append(str, hex[tmp]);
    }
    
    return str;
}

/* K&R, section 3.5 */
/* reverse: reverse string s in place */
void reverse(char s[]) {
    int c, i, j;
    for (i = 0, j = strlen(s)-1; i < j; i++, j--) {
        c = s[i];
        s[i] = s[j];
        s[j] = c;
    }
}

/* K&R, section 2.3 */
/* strlen: return length of s */
int strlen(char s[]) {
    int i = 0;
    while (s[i] != '\0')
        ++i;
    return i;
}

void append(char s[], char n) {
    int len = strlen(s);
    s[len] = n;
    s[len+1] = '\0';
}

void backspace(char s[]) {
    int len = strlen(s);
    s[len-1] = '\0';
}

/* K&R, section 5.5 */
/* strcmp: return <0 if s<t, 0 if s==t, >0 if s>t */
int strcmp(char *s, char *t) {
    int i;
    for (i = 0; s[i] == t[i]; i++)
        if (s[i] == '\0')
            return 0;
    return s[i] - t[i];
}

/**
 * @brief      Returns a char** containing all substrings of a_str split by a_delim
 *
 * @param      a_str    String to split
 * @param[in]  a_delim  Delimiter to split string by
 *
 * @return     char** containing all substrings. <b>char** and each char* must be freed</b>
 */
char** str_split(char* a_str, const char a_delim) {
    int count = 0;
    char current = a_str[0];
    int i = 0;
    while(current != '\0') {
        if(current == a_delim) count++;
        
        i++;
        current = a_str[i];
    }

    char** output = malloc((count+1)*sizeof(char*));
    if(count == 0) { 
        output[0] = a_str;
        return output; 
    }
    

    current = a_str[0];
    count = 0;
    i = 0;
    int last = 0;
    while(current != '\0') {
        if(current == a_delim) {
            output[count] = malloc(sizeof(char)*(i-last));
            int j = 0;
            while(j < (i-last)) {
                output[count][j] =  a_str[last+j];
                j++;
            }
            output[count][j-1] = '\0';
            last = i;
            count++;
            current = a_str[i++];
        } else {
            current = a_str[i++];
        }
    }

    output[count] = malloc(sizeof(char)*(i-last));
    int j = 0;
    while(j < (i-last)) {
        output[count][j] =  a_str[last+j];
        j++;
    }
    output[count][j] = '\0';
    output[count+1] = malloc(sizeof(char));
    output[count+1] = '\0';
    return output;
}

int8_t character_exists(char char_to_find, char* string_to_search) {
    char* z = string_to_search;
    uint32_t d = 0;
    while(z[d] != 0) {
        if(z[d++] == char_to_find) return d;
    }
    return -1;
}