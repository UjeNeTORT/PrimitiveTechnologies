#ifndef TEXT_BUF_H
#define TEXT_BUF_H

/**
 * @brief read text from file to buf. then put ptrs to str beginnings to text arr
 *
 * @param [in]  file     in-filename
 * @param [out] text     pointer to array of pointers to different parts (new lines) of buffer
 * @param [out] buf      pointer to buffer
 *
 * @return number of lines readen
 *
*/
size_t ReadText(const char * file, char ***text, char **buf);

/**
 * @brief write in file no more then n_lines strings pointers to which are in text
 *
 * @param [in]  file    out-filename
 * @param [in]  mode    "w", "a" and other modes of writing to file
 * @param [out] text    array of pointers to different parts (new lines) of buffer
 * @param [in]  n_lines nmber of lines
 *
*/
void WriteText(const char * const file, const char * const mode, const char **text, int n_lines);

/**
 * @brief write in file no more then n_lines strings from buf (from \0 to \0)
 *
 * @param [in]  file    out-filename
 * @param [in]  mode    "w", "a" and other modes of writing to file
 * @param [out] buf     pointer to buffer
 * @param [in]  n_lines nmber of lines
 *
*/
void WriteBuf(const char * const file, const char * const mode, const char *buf, int n_lines);

/**
 * @brief set all \rs to \0 (if no \r, set \ns to \0)
 *
 * @param [in] buf     buffer
 * @param [in] n_lines number of lines
 *
 * @return pointer to array of pointers to different parts (new lines) of buffer
*/
char **ParseLines(char *buf, size_t n_lines);

/**
 * @brief free text (шок)
 *
 * @param [in] text  memory to free
*/
void FreeText(void *text);

/**
 * @brief free buf (шок)
 *
 * @param [in] buf  memory to free
*/
void FreeBuf(void *buf);

#endif // TEXT_BUF_H
