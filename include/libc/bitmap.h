// https://codereview.stackexchange.com/questions/8213/bitmap-implementation
#define BIT (8*sizeof(byte))
#define BITMAP_NOTFOUND -1

typedef enum{false_t=0, true_t} bool_t;
typedef unsigned char byte;

bool_t bitmapGet   (byte *, int);
void bitmapSet   (byte *, int);
void bitmapReset (byte *, int);
int  bitmapSearch(byte *, bool_t, int, int);