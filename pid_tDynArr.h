/**************************************************************************
 * Date:        2/24/2020
 * Description: Defines functions for managing dynamic pid_t type arrays
 *              including:
 *
 *               initializing an array
 *               reseting array elements
 *               returning the array index of an element with a given value
 *               getting a dynamic one-dimensional array of a given size
 *               double the size of an existing dynamic array 
 *
 **************************************************************************/

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>  
#include <sys/types.h> /* includes pid_t type */

/* Initialize the array of background PIDs to this value, and reset to this
 * value when a background process has completed
 */
#define BG_UNDEFINED -100

/* Initialize each element in a pid_t array to the value BG_UNDEFINED 
 * input: the array that is to be initialized: pid_t[]
 *        the size of the array: int
 */
void initializeBgPidArray(pid_t bgPidReturn[], int pidReturnCapacity);

/* Returns the index of the array element with the value "element", where
 * it is assumed that all elements are unique in value if they are not
 * equal to BG_UNDEFINED. Note: returns -1 if can't find the element.
 * input: the array that is to be searched: pid_t[]
 *        the value that is to be searched for: pid_t
 *        the size of the array: int
 * output: the index of the array element or -1 if not found
 */
int pidArrayIndex(pid_t pidReturn[], pid_t element, int pidReturnCapacity);

/* Reset an array element to the value BG_UNDEFINED. Note that it is
 * assumed that all elements are unique in value if they are not equal
 * to BG_UNDEFINED.
 * input: the array that the element is in
 *        the value of the element
 *        the size of the array
 */
void resetPidArrayElement(pid_t pidReturn[], pid_t element, int pidReturnCapacity);

/* Create a one-dimensional dynamic array of size "size" and type pid_t
 * input: the array size: int
 * output: a one-dimensional dynamic array: pid_t*
 */
pid_t *getPidReturnArray(int size);

/* Double the size of a one-dimensional dynamic array of type pid_t and
 * return the new array size.
 * input: the array that is to be increased: pid_t &*
 *        the size of the array: int
 * output: the new size of the array: int
 */
int increasePidReturnArray(pid_t **arr, int oldCapacity);