/**************************************************************************
 * Date:        2/24/2020
 * Description: Implements functions for managing dynamic pid_t type arrays
 *              including:
 *
 *               initializing an array
 *               reseting array elements
 *               returning the array index of an element with a given value
 *               getting a dynamic one-dimensional array of a given size
 *               double the size of an existing dynamic array 
 *
 **************************************************************************/

#include "pid_tDynArr.h"

/* Initialize each element in a pid_t array to the value BG_UNDEFINED */
void initializeBgPidArray(pid_t bgPidReturn[], int pidReturnCapacity)
{
    int i;
    for (i = 0; i < pidReturnCapacity; i++)
    {
        bgPidReturn[i] = BG_UNDEFINED;
    }
}

/* Returns the index of the array element with the value "element", where
 * it is assumed that all elements are unique in value if they are not
 * equal to BG_UNDEFINED.
 */
int pidArrayIndex(pid_t pidReturn[], pid_t element, int pidReturnCapacity)
{
    int i = 0 ;

    /* increment through array until find a match */
    while (i < pidReturnCapacity && pidReturn[i] != element )
    {
        ++i;
    }

    if (i < pidReturnCapacity)
    {
        return i;  /* return the matching element index */
    }
    
    return -1; /* return -1 if can't find the element */
}

/* Reset an array element to the value BG_UNDEFINED. Note that it is
 * assumed that all elements are unique in value if they are not equal
 * to BG_UNDEFINED.
 */
void resetPidArrayElement(pid_t pidReturn[], pid_t element, int pidReturnCapacity)
{
    int index;

    /* get the index of the array element with value "element"*/
    index = pidArrayIndex(pidReturn, element, pidReturnCapacity);

    if (index != -1 ) /* if index successfully found */
    {
       pidReturn[index] = BG_UNDEFINED; /* reset array element */
    }

}

/* Create a one-dimensional dynamic array of size "size" and type pid_t */
pid_t *getPidReturnArray(int size)
{
    pid_t *temp;  /* holds a dynamic array */

    /* create a dynamic array of type pid_t */
    temp = (pid_t *)malloc(size*sizeof(pid_t));

    if (temp == NULL) /* print error and exit if unsuccessful */
    {
        fprintf(stderr, "Malloc not successful\n");
        exit(1);
    }

    return temp; /* return the array */
}

/* Double the size of a one-dimensional dynamic array of type pid_t and
 * return the new array size.
 */
int increasePidReturnArray(pid_t **arr, int oldCapacity)
{
    int i;
    int newCapacity = 2*oldCapacity; /* double the array size */
    pid_t *temp; /* holds the larger array with the new capacity */

    /* make a dynamic array that is double the size */
    temp = (pid_t *)malloc(newCapacity*sizeof(pid_t));

    /*initialize the newly made dynamic array*/
    initializeBgPidArray(temp, newCapacity);

    /*copy over values from the original array to the new one */
    for (i = 0; i< oldCapacity; i++)
        {
            temp[i] = (*arr)[i];
        }

    free(*arr);  /* delete the original array */

    *arr = temp;  /* assign the input array to the new array */

    return newCapacity; /* return the new array capacity */
}
