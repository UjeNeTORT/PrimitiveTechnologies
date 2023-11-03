#include <assert.h>
#include <ctype.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "text_buf.h"

//----------------------------------------------------
/**
 * @brief reads info to buffer from file
 *
 * @param [in]  file     filename
 * @param [out] buf      buffer pointer
 *
 * @return number of lines in file
*/
static int ReadBuf(const char * const file, char **buf);

//----------------------------------------------------
/**
 * @brief calculates size of the windows-saved file not bigger than 2GB (may not work properly on other OS)
 *
 *
 * @param [in] file pointer to a file
 *
 * @return file size in bytes
 *
 * @details
 * ONLY <= 2GB files accepted
*/
static long GetFileSize(FILE *file);

//----------------------------------------------------
/**
 * @brief counts number of lines in buffer
 *
 * @param [in] buf buffer
 *
 * @return number of \\ns in the buffer
*/
static int CntNewLine(const char *buf);

//----------------------------------------------------

size_t ReadText(const char * file, char ***text, char **buf) {

    assert (file);
    assert (text);
    assert (buf);

    if (!file) {
        fprintf(stderr, "ReadText: file null received\n");
        // return INT_MAX;
    }

    if (!text) {
        fprintf(stderr, "ReadText: text null received\n");
        // return INT_MAX;
    }

    if (!buf) {
        fprintf(stderr, "ReadText: buf null received\n");
        // return INT_MAX;
    }

    int n_lines = ReadBuf(file, buf);

    *text = (char **) ParseLines(*buf, n_lines);

    return n_lines;
}

//----------------------------------------------------

int ReadBuf(const char * const file, char **buf) {
    assert (file);
    assert (buf);

    if (!file) {
        fprintf(stderr, "ReadBuf: no file given\n");
        // return INT_MAX;
    }

    if (!buf) {
        fprintf(stderr, "ReadBuf: null pointer to buf\n");
        // return INT_MAX;
    }

    FILE *fin = fopen(file, "rb");

    long f_size = GetFileSize(fin);

    *buf = (char *) calloc(f_size + 1, sizeof(char));

    if (fread(*buf, sizeof(char), f_size, fin) != f_size)
    {
        fprintf(stderr, "ReadBuf: readen != f_size\n");
        abort();
    }

    fclose(fin);

    return CntNewLine(*(char * const *) buf);
}

//----------------------------------------------------

char **ParseLines(char *buf, size_t n_lines) {
    assert (buf);

    if (!buf) {
        fprintf(stderr, "ParseLines: null pointer to buf received. Destroying the notebook.\n");
        return NULL;
    }

    char **l_ptrs = (char **) calloc(n_lines + 1, sizeof(char *));
    char **l_ptrs_ansc = l_ptrs;

    *l_ptrs_ansc++ = buf;

    // strchr is NOT suitable for this purpose
    while (*buf) {
        if (*buf == '\r') {
            *buf++ = '\0';

            // skip \n after \r

            *l_ptrs_ansc++ = ++buf;
        }
        else if (*buf == '\n'){
            *buf++ = '\0';
            *l_ptrs_ansc++ = buf;
        }
        else {
            buf++;
        }
     }

        return l_ptrs;
}

//----------------------------------------------------

void FreeText(void *text) {
    assert (text);

    if (!text) {
        fprintf(stderr, "FreeText: null pointer to text received");

        return ;
    }

    free(text);
}

//----------------------------------------------------

void FreeBuf(void *buf) {
    assert (buf);

    if (!buf) {
        fprintf(stderr, "FreeBuf: null pointer to buf received");
        return ;
    }

    free(buf);
}

//----------------------------------------------------

void WriteText(const char * const file, const char * const mode, const char **text, int n_lines) {
    assert (file);
    assert (text);
    assert (mode);
    assert (n_lines >= 0);

    if (!file) {
        fprintf(stderr, "WriteText: null pointer to file received\n");
        return ;
    }

    if (!mode) {
        fprintf(stderr, "WriteText: no mode received");
        return ;
    }

    if (!text) {
        fprintf(stderr, "WriteText: null pointer to text received\n");
        return ;
    }

    if (n_lines < 0) {
        fprintf(stderr, "WriteText: n_lines < 0\n");
        return ;
    }

    FILE *fout = fopen(file, mode);

    while (n_lines-- > 0)
        fprintf(fout, "%s\n", *text++);

    fclose(fout);

}

//----------------------------------------------------

void WriteBuf(const char * const file, const char * const mode, const char *buf, int n_lines) {
    assert (file);
    assert (buf);
    assert (n_lines >= 0);

    if (!file) {
        fprintf(stderr, "WriteBuf: null pointer to file received\n");
        return ;
    }

    if (!mode) {
        fprintf(stderr, "WriteBuf: no mode received");
        return ;
    }

    if (!buf) {
        fprintf(stderr, "WriteBuf: null pointer to buffer received\n");
        return ;
    }

    if (n_lines < 0) {
        fprintf(stderr, "WriteBuf: n_lines < 0\n");
        return ;
    }

    FILE *fout = fopen(file, mode);

    while (n_lines-- > 0) {
        fprintf(fout, "%s\n", buf);

        // skip string + \0 + \n
        buf += strlen(buf) + 2;
    }

    fclose(fout);
}

//----------------------------------------------------

long GetFileSize(FILE *file) {
    assert (file);

    if (!file) {
        fprintf(stderr, "GetFileSize: null instead of file received\n");
        // return INT_MAX;
    }

    fseek(file, 0, SEEK_END);
    long res = ftell(file);
    rewind(file);

    return res;
}

//----------------------------------------------------

int CntNewLine(const char *buf) {
    assert (buf);

    if (!buf) {
        fprintf(stderr, "CntNewLine: null pointer to buffer received\n");
        // return INT_MAX;
    }

    int n_cnt = 0;

    while (*buf++)
        if (*buf == '\n') n_cnt++;

    return n_cnt;
}
