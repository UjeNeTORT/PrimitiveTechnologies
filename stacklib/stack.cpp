#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "my_hash.h"
#include "stack.h"

enum POISON_OUT {
    POISON_ERR_SIDE = -1,
    POISON_NO_ERR   =  0,
    POISON_ERR      =  1
};

enum DATA_CLLC_OUT {
    DATA_CLLC_ERR_SIDE = -1,
    DATA_CLLC_NO_ERR   =  0,
    DATA_CLLC_ERR      =  1
};

enum DATA_RLLC_OUT {
    DATA_RLLC_ERR_SIDE = -1,
    DATA_RLLC_NO_ERR   =  0,
    DATA_RLLC_ERR      =  1
};

//-------------------------------------------------------------------------------------
/**
 * @param [out] data     stack data structure
 * @param [in]  capacity stack capacity
 *
 * @brief calloc memory of size capacity * sizeof(elem_t) for data ptr
*/
static enum DATA_CLLC_OUT
              StackDataCalloc    (stk_data * data, int capacity);

//-------------------------------------------------------------------------------------
/**
 * @param [out] data stack data structure
 * @param [in] new_capacity new required stack data size
 *
 * @brief reallocs pointers of data structure and canaries
*/
static enum DATA_RLLC_OUT
              StackDataRealloc   (stk_data * data, int new_capacity);

//-------------------------------------------------------------------------------------
/**
 * @param [out] stk        stack structure
 * @param [in]  debug_info contains info about place from which we got into function
 *
 * @brief sets values of stack data to poison
*/
static enum POISON_OUT
              StackPoison        (stack *stk, stk_debug_info debuf_info);

//-------------------------------------------------------------------------------------
/**
 * @param [out] data     data structure
 * @param [in]  capacity capacity
*/
static void   FreeData           (stk_data *data, int capacity);

//-------------------------------------------------------------------------------------
/**
 * @param [in] err_vector vector with errors 1011100 and so on (1 - error, 0 - no error)
 *
 * @return pointer to string with all the errors messages
 *
 * @brief concatenates error message based on what the errors from err_vector are
*/
static char * FormErrMsg         (size_t err_vector);

//-------------------------------------------------------------------------------------
/**
 * @param [in] fout file to contain log
 * @param [in] stk  stack pointer
 *
 * @brief prints data values from stack for dump
*/
static void   PrintStackDataDump (FILE *fout, const stack *stk);

//-------------------------------------------------------------------------------------
/**
 * @param [in] stk        stack pointer
 * @param [in] debug_info contains info about place from which we got into function
 *
 * @return new capacity if it is to be changed
 *
 * @brief calculates new capacity for stack based on what stack size and capacity are
*/
static int    GetNewCapacity     (stack *stk, stk_debug_info debuf_info);
//-------------------------------------------------------------------------------------
/**
 *
*/
static void UpdStackHash(stack *stk);

// in order not to create another one every time i need it
size_t err_vector = 0;
char * err_msg = (char *) calloc(MAX_ERR_MSG_STRING, sizeof(char *)); //

//-------------------------------------------------------------------------------------
enum CTOR_OUT StackCtor(stack *stk, int capacity, stk_debug_info debug_info) {

    //dont use StackErr because we are in constructor and no way it breaks
    assert(stk);
    if (!stk)
        return CTOR_NULL_STK;

    //assume that stk didnt go through constructor before, if yes - abort()
    size_t err = StackErr(stk);
    if (!StackErr(stk)) {
        STACK_DUMP(LOG_FILE, stk, err, debug_info);
        fprintf(stderr, "attempt to call constructor 2 times\n");
        abort();
    }

    #if (defined(STACK_CANARY_PROTECT))

    stk->l_canary = LEFT_CHICK;
    stk->r_canary = RIGHT_CHICK;

    #endif // defined(STACK_CANARY_PROTECT)

    stk->size = 0;
    stk->capacity      = capacity;
    stk->init_capacity = capacity;

    StackDataCalloc(&stk->data, capacity);

    UpdStackHash(stk);
    ASSERT_STACK(stk, debug_info);
    if (StackErr(stk))
        return CTOR_ERR;

    StackPoison(stk, debug_info);

    UpdStackHash(stk);
    ASSERT_STACK(stk, debug_info);
    if (StackErr(stk))
        return CTOR_ERR;

    return CTOR_NO_ERR;
}

//-------------------------------------------------------------------------------------
enum REALLC_OUT StackRealloc(stack *stk, int new_capacity, stk_debug_info debug_info) {

    ASSERT_STACK(stk, debug_info);
    if (StackErr(stk))
        return REALLC_ERR_SIDE;

    if (stk->capacity != new_capacity) {

        StackDataRealloc(&stk->data, new_capacity);
        stk->capacity = new_capacity;
    }

    UpdStackHash(stk);
    StackPoison(stk, debug_info);

    UpdStackHash(stk);
    ASSERT_STACK(stk, debug_info);
    if (StackErr(stk))
        return REALLC_ERR;

    return REALLC_NO_ERR;
}

//-------------------------------------------------------------------------------------
enum DTOR_OUT StackDtor(stack *stk, stk_debug_info debug_info) {

    ASSERT_STACK(stk, debug_info);
    if (StackErr(stk))
        return DTOR_ERR_SIDE;

    #if (defined(STACK_CANARY_PROTECT))

        stk->l_canary = 0;
        stk->r_canary = 0;

    #endif // defined(STACK_CANARY_PROTECT)

    FreeData(&stk->data, stk->capacity);

    stk->size     = -1;
    stk->capacity = -1;

    #if (defined(STACK_HASH_PROTECT))

        stk->stk_hash_sum = 0;

    #endif // defined(STACK_HASH_PROTECT)

    #if (defined(DATA_HASH_PROTECT))

        stk->data_hash_sum = 0;

    #endif // defined(DATA_HASH_PROTECT)

    stk = NULL;

    return DTOR_DESTR;
}

//-------------------------------------------------------------------------------------
enum PUSH_OUT StackPush(stack *stk, Elem_t value, stk_debug_info debug_info) {

    ASSERT_STACK(stk, debug_info);
    if (StackErr(stk))
        return PUSH_ERR_SIDE;

    int new_capacity = GetNewCapacity(stk, debug_info);
    if (stk->capacity != new_capacity)
        StackRealloc(stk, new_capacity, debug_info);

    stk->data.buf[stk->size] = value;
    stk->size++;

    UpdStackHash(stk);
    ASSERT_STACK(stk, debug_info);
    if (StackErr(stk))
        return PUSH_ERR;

    return PUSH_NO_ERR;
}

//-------------------------------------------------------------------------------------
Elem_t StackPop(stack *stk, enum POP_OUT *err, stk_debug_info debug_info) {

    *err = POP_NO_ERR;

    ASSERT_STACK(stk, debug_info);
    if (StackErr(stk))
        *err = POP_ERR_SIDE;

    int new_capacity = GetNewCapacity(stk, debug_info);

    if (stk->capacity != new_capacity) {
        StackRealloc(stk, new_capacity, debug_info);
    }

    stk->size--;

    Elem_t ret_value = stk->data.buf[stk->size];

    UpdStackHash(stk);
    ASSERT_STACK(stk, debug_info);
    if (StackErr(stk))
        *err = POP_ERR;

    StackPoison(stk, debug_info);

    UpdStackHash(stk);
    ASSERT_STACK(stk, debug_info);
    if (StackErr(stk))
        *err = POP_ERR;

    return ret_value;
}

//-------------------------------------------------------------------------------------
static enum DATA_CLLC_OUT StackDataCalloc (stk_data * data, int capacity) {

    assert (data);

    if (!data)
        return DATA_CLLC_ERR_SIDE;

    if (capacity <= 0) {

        #if (defined(DATA_CANARY_PROTECT))

            data->l_canary = NULL;
            data->r_canary = NULL;

        #endif // defined(DATA_CANARY_PROTECT)

        data->buf = NULL;

        return DATA_CLLC_ERR_SIDE;
    }

    #if (defined(DATA_CANARY_PROTECT))

        int padding = (capacity * sizeof(Elem_t)) % sizeof(Canary_t); // for alignment

        data->buf = (Elem_t * ) calloc(capacity * sizeof(Elem_t) + 2 * sizeof(Canary_t) + padding, 1);

        if (!data->buf)
            return DATA_CLLC_ERR;

        data->l_canary = (Canary_t *) data->buf;
        *data->l_canary = LEFT_CHICK;

        data->buf = (Elem_t *) ((Canary_t *) data->buf + 1);

        data->r_canary = (Canary_t *) ((char *) data->buf + capacity * sizeof(Elem_t) + padding); // alignment
        *data->r_canary = RIGHT_CHICK;

    #else

        data->buf = (Elem_t *) calloc(capacity, sizeof(Elem_t));

        if (!data->buf)
            return DATA_CLLC_ERR;

    #endif // defined(DATA_CANARY_PROTECT)

    return DATA_CLLC_NO_ERR;
}

//-------------------------------------------------------------------------------------
static enum DATA_RLLC_OUT StackDataRealloc (stk_data *data, int new_capacity) {

    assert(data);
    if (!data)
        return DATA_RLLC_ERR_SIDE;

    #if (defined(DATA_CANARY_PROTECT))

        int padding = (new_capacity * sizeof(Elem_t)) % sizeof(Canary_t); // for alignment

        data->buf = (Elem_t * ) realloc(data->l_canary, new_capacity * sizeof(Elem_t) + 2 * sizeof(Canary_t) + padding);

        if (!data->buf)
            return DATA_RLLC_ERR;

        data->l_canary = (Canary_t * ) data->buf;
        *data->l_canary = LEFT_CHICK;

        data->buf = (Elem_t *) ((Canary_t *) data->buf + 1);

        data->r_canary = (Canary_t *) ((char *) data->buf + new_capacity * sizeof(Elem_t) + padding); // alignment
        *data->r_canary = RIGHT_CHICK;

        // StackPoison(stk, debug_info); // calloc analogy

    #else

        data->buf = (Elem_t * ) realloc(data->buf, new_capacity * sizeof(Elem_t));

        assert(data->buf);
        if (!data->buf)
            return DATA_RLLC_ERR;

    #endif // defined(DATA_CANARY_PROTECT)

    return DATA_RLLC_NO_ERR;
}

//-------------------------------------------------------------------------------------
static void FreeData (stk_data *data, int capacity) {

    assert(data); // TODO
    if (!data)
        return;

    for (int i = 0; i < capacity; i++) {
        data->buf[i] = POISON; // DEAD MEMORY
    }

    data->buf = NULL;

    #if (defined(DATA_CANARY_PROTECT))

        *data->l_canary = 0;
        *data->r_canary = 0;

         free(data->l_canary);

         data->l_canary = NULL;
         data->r_canary = NULL;

    #endif // defined(DATA_CANARY_PROTECT)
}

//-------------------------------------------------------------------------------------
void StackDump(const char  * const fname,
               const stack *       stk,
               size_t              err_vec,
               const char  *       stk_name,
               const char  * const err_file,
               int                 err_line,
               const char  * const err_func,
               stk_debug_info      debug_info) {

    assert (stk);
    assert (fname);
    assert (stk_name);
    assert (err_file);
    assert (err_func);

    FILE *fout = fopen(fname, "a");

    assert (fout);

    fprintf(fout, "stack[%p] %s from %s (%d)\n"
                  "             called from %s (%d) %s\n", stk, debug_info.stk_name, debug_info.filename, debug_info.line, err_file, err_line, err_func);
    fprintf(fout, "{\n");

    fprintf(fout, "size     = %d (MX_STK = %d)\n", stk->size, MX_STK);
    fprintf(fout, "capacity = %d\n", stk->capacity);

    #if (defined(STACK_CANARY_PROTECT))

        fprintf(fout, "left  stack canary[%p] = %lu (%s)\n", &stk->l_canary, stk->l_canary, (stk->l_canary == LEFT_CHICK)  ? "ok" : "corrupted");
        fprintf(fout, "right stack canary[%p] = %lu (%s)\n", &stk->r_canary, stk->r_canary, (stk->r_canary == RIGHT_CHICK) ? "ok" : "corrupted");

    #endif // defined(STACK_CANARY_PROTECT)

    #if (defined(STACK_HASH_PROTECT))

        fprintf(fout, "stack hash = %lu\n", stk->stk_hash_sum);

    #endif // defined(STACK_HASH_PROTECT)

    err_msg = FormErrMsg(err_vec);
    if (*err_msg)
        fprintf(fout, "ERRORS: %s\n", err_msg);

    PrintStackDataDump(fout, stk);

    fprintf(fout, "}\n");

    fprintf(fout, "------------------------------------------------------\n");

    fclose(fout);
}

//-------------------------------------------------------------------------------------
//verificator
size_t StackErr(stack *stk) {

    size_t errors = 0;

    if (!stk) {
                                            errors |=    1;
        return errors;
    }

    if (!stk->data.buf) {
                                            errors |=    2;
        return errors;
    }
    if (stk->size < 0)                      errors |=    4;
    if (stk->capacity <= 0)                 errors |=    8;
    if (stk->size > stk->capacity)          errors |=   16;
    if (stk->capacity > MX_STK)             errors |=   32;
    if (stk->init_capacity > stk->capacity) errors |=   64;

    #if (defined(DATA_CANARY_PROTECT))

    if (!stk->data.l_canary || *stk->data.l_canary != LEFT_CHICK)
                                            errors |=  128;

    if (!stk->data.r_canary || *stk->data.r_canary != RIGHT_CHICK)
                                            errors |=  256;

    #endif // defined(DATA_CANARY_PROTECT)

    #if (defined(STACK_CANARY_PROTECT))

        if (stk->l_canary != LEFT_CHICK)    errors |=  512;
        if (stk->r_canary != RIGHT_CHICK)   errors |= 1024;

    #endif // defined(STACK_CANARY_PROTECT)

    #if (defined(STACK_HASH_PROTECT))

        Hash_t new_stack_hash = 0;

        new_stack_hash = HashMod(&stk->data, sizeof(stk->data) + sizeof(stk->size) + sizeof(stk->capacity) + sizeof(stk->init_capacity));

        if (stk->stk_hash_sum != new_stack_hash) {
            errors |= 4096;
        }

    #endif // defined(STACK_HASH_PROTECT)

    #if (defined(DATA_HASH_PROTECT))

        Hash_t new_data_hash = 0;

        new_data_hash = HashMod(stk->data.buf, stk->capacity * sizeof(Elem_t));

        if (new_data_hash != stk->data_hash_sum) {
            errors |= 2048;
        }

    #endif // defined(DATA_HASH_PROTECT)

    return errors;
}

//-------------------------------------------------------------------------------------
/*anscillary function for StackDump

is used to form error messages
*/
static char * FormErrMsg(size_t err_vec) {

    strcpy(err_msg, "");

    if (err_vec &  1)
        ADD_ERR_MSG(err_msg, "stack null pointer");

    if (err_vec &  2)
        ADD_ERR_MSG(err_msg, "buf null pointer");

    if (err_vec &  4)
        ADD_ERR_MSG(err_msg, "size < 0");

    if (err_vec &  8)
        ADD_ERR_MSG(err_msg, "capacity <= 0");

    if (err_vec & 16)
        ADD_ERR_MSG(err_msg, "size >= capacity");

    if (err_vec & 32)
        ADD_ERR_MSG(err_msg, "capacity bigger than max stack size");

    if (err_vec & 64)
        ADD_ERR_MSG(err_msg, "init capacity is bigger than capacity");

    #if (defined(DATA_CANARY_PROTECT))

        if (err_vec & 128)
            ADD_ERR_MSG(err_msg, "left canary: data attacked from the left");

        if (err_vec & 256)
            ADD_ERR_MSG(err_msg, "right canary: data attacked from the right");

    #endif // defined(DATA_CANARY_PROTECT)

    #if (defined(STACK_CANARY_PROTECT))

        if (err_vec & 512)
            ADD_ERR_MSG(err_msg, "left canary: stack attacked from the left");

        if (err_vec & 1024)
            ADD_ERR_MSG(err_msg, "right canary: stack attacked from the right");

    #endif // defined(STACK_CANARY_PROTECT)

    #if (defined(DATA_HASH_PROTECT))

        if (err_vec & 2048)
            ADD_ERR_MSG(err_msg, "data hashes does not match, sudden attack");

    #endif // defined(DATA_HASH_PROTECT)

    #if (defined(STACK_HASH_PROTECT))

        if (err_vec & 4096)
            ADD_ERR_MSG(err_msg, "stack hash does not match, sudden attack");

    #endif // defined(STACK_HASH_PROTECT)

    strcat(err_msg, "\n");

    return err_msg;
}

//-------------------------------------------------------------------------------------
static int GetNewCapacity(stack *stk, stk_debug_info debug_info) {

    ASSERT_STACK(stk, debug_info);

    if (stk->size + 1 >= (int) stk->capacity * 3 / 4)
        return stk->capacity * 2;

    if (stk->size - 1 <= (int) stk->capacity / 4 + 1 && stk->capacity > stk->init_capacity)
        return stk->capacity / 2;

    UpdStackHash(stk);
    ASSERT_STACK(stk, debug_info);

    return stk->capacity;
}

//-------------------------------------------------------------------------------------
// returns pointer to string which contents fully prepared string with every element of data
static void PrintStackDataDump(FILE *fout, const stack *stk) {

    /* here we dont use ASSERT_STACK because we use this func in debug and this means that stack is already not ok */
    assert (stk);
    assert (fout);

    #if (defined(DATA_HASH_PROTECT))

    fprintf(fout, "stack_hash_sum = %lu\n", stk->stk_hash_sum);
    fprintf(fout, "data_hash_sum  = %lu\n", stk->data_hash_sum);

    #endif // defined(DATA_HASH_PROTECT)

    #if (defined(DATA_CANARY_PROTECT))

    fprintf(fout, "l_canary[%p] = %lu (%s)\n", stk->data.l_canary, (stk->data.l_canary) ? *stk->data.l_canary : 0, (stk->data.l_canary) ? ((*stk->data.l_canary == LEFT_CHICK)  ? "ok" : "corrupted") : "ptr null");
    fprintf(fout, "r_canary[%p] = %lu (%s)\n", stk->data.r_canary, (stk->data.r_canary) ? *stk->data.r_canary : 0, (stk->data.r_canary) ? ((*stk->data.r_canary == RIGHT_CHICK) ? "ok" : "corrupted") : "ptr null");

    #endif // defined(DATA_CANARY_PROTECT)

    fprintf(fout, "buf[%p]\n"
                  "\t{\n", stk->data.buf);

    if (!stk->data.buf) {
        fprintf(fout, "\t\tbuf null pointer, nothing to look at\n");
    }
    else if (stk->capacity <= 0) {
        fprintf(fout, "\t\tstack capacity is 0 or less, cant print it\n");
    }
    else if (stk->capacity > MX_STK) {
        fprintf(fout, "\t\tstack capacity is bigger than allowed (%d) memory allocation did not happen\n", MX_STK);
    }
    else {

        for (int i = 0; i < stk->capacity; i++) {

            if (i < stk->size) {
                fprintf(fout, "\t\t*[%4d] = %6d\n", i, stk->data.buf[i]);
            }
            else {

                fprintf(fout, "\t\t [%4d] = %6d %s\n", i, stk->data.buf[i], (stk->data.buf[i] == POISON) ? "(POISON)" : "");
            }
        }
    }

    fprintf(fout, "\t}\n");
}

//-------------------------------------------------------------------------------------
static enum POISON_OUT StackPoison(stack *stk, stk_debug_info debug_info) {

    ASSERT_STACK(stk, debug_info);
    if (StackErr(stk))
        return POISON_ERR_SIDE;

    int cnt = stk->size;

    while (cnt < stk->capacity)
        stk->data.buf[cnt++] = POISON;

    UpdStackHash(stk);
    ASSERT_STACK(stk, debug_info);
    if (StackErr(stk))
        return POISON_ERR;

    return POISON_NO_ERR;
}

//-------------------------------------------------------------------------------------
stk_debug_info UpdDebugInfo(const char *stk_name, const char *filename, int line) {

    stk_debug_info debug_info = {};

    debug_info.stk_name = stk_name;
    debug_info.filename = filename;
    debug_info.line     = line;

    return debug_info;
}

//-------------------------------------------------------------------------------------
static void UpdStackHash(stack *stk) {

    assert(stk);

    #if (defined(DATA_HASH_PROTECT))

        stk->data_hash_sum = HashMod(stk->data.buf, stk->capacity * sizeof(Elem_t));

    #endif // defined(DATA_HASH_PROTECT)


    #if (defined(STACK_HASH_PROTECT))

        stk->stk_hash_sum = HashMod(&stk->data, sizeof(stk->data) + sizeof(stk->size) + sizeof(stk->capacity) + sizeof(stk->init_capacity));

    #endif // defined(STACK_HASH_PROTECT)
}
